import Stripe from 'stripe';

const PRODUCT_SLUG = 'pilgrim-of-the-thorn';
const PRODUCT_NAME = 'Pilgrim of the Thorn';
const LICENSE_ALPHABET = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
const EMAIL_CLAIM_TIMEOUT_MS = 5 * 60 * 1000;

type SaleRow = {
    email: string;
    email_sent_at: string | null;
    email_status: 'pending' | 'sending' | 'sent' | 'failed';
    license_key: string;
};

function stripeClient(secretKey: string): Stripe {
    return new Stripe(secretKey, {
        httpClient: Stripe.createFetchHttpClient(),
        maxNetworkRetries: 2,
    });
}

function json(data: unknown, init: ResponseInit = {}): Response {
    const headers = new Headers(init.headers);
    headers.set('content-type', 'application/json; charset=utf-8');
    headers.set('cache-control', 'no-store');
    return Response.json(data, { ...init, headers });
}

function stripeId(value: string | { id: string } | null): string | null {
    if (!value) {
        return null;
    }

    return typeof value === 'string' ? value : value.id;
}

function createLicenseKey(): string {
    const bytes = crypto.getRandomValues(new Uint8Array(20));
    const characters = Array.from(bytes, (byte) => LICENSE_ALPHABET[byte % LICENSE_ALPHABET.length]);
    const groups = Array.from({ length: 4 }, (_, index) => characters.slice(index * 5, index * 5 + 5).join(''));
    return `POTT-${groups.join('-')}`;
}

function escapeHtml(value: string): string {
    return value
        .replaceAll('&', '&amp;')
        .replaceAll('<', '&lt;')
        .replaceAll('>', '&gt;')
        .replaceAll('"', '&quot;')
        .replaceAll("'", '&#039;');
}

function maskEmail(email: string): string {
    const [local, domain] = email.split('@');
    if (!local || !domain) {
        return 'your email address';
    }

    return `${local.slice(0, 2)}${'*'.repeat(Math.max(2, local.length - 2))}@${domain}`;
}

async function createCheckoutSession(): Promise<Response> {
    return json({ error: 'Purchases are coming soon.' }, { status: 503 });
}

async function insertSale(session: Stripe.Checkout.Session, email: string, env: Env): Promise<void> {
    const now = new Date().toISOString();
    await env.DB.prepare(
        `INSERT OR IGNORE INTO sales (
      stripe_checkout_session_id,
      stripe_payment_intent_id,
      stripe_customer_id,
      email,
      amount_total,
      currency,
      license_key,
      payment_status,
      email_status,
      created_at,
      updated_at
    ) VALUES (?, ?, ?, ?, ?, ?, ?, 'paid', 'pending', ?, ?)`,
    )
        .bind(
            session.id,
            stripeId(session.payment_intent),
            stripeId(session.customer),
            email,
            session.amount_total,
            session.currency,
            createLicenseKey(),
            now,
            now,
        )
        .run();
}

async function claimEmailDelivery(sessionId: string, env: Env): Promise<SaleRow | null> {
    const now = new Date().toISOString();
    const staleBefore = new Date(Date.now() - EMAIL_CLAIM_TIMEOUT_MS).toISOString();
    const result = await env.DB.prepare(
        `UPDATE sales
      SET email_status = 'sending', email_attempted_at = ?, updated_at = ?
      WHERE stripe_checkout_session_id = ?
        AND email_sent_at IS NULL
        AND (
          email_status IN ('pending', 'failed')
          OR (email_status = 'sending' AND email_attempted_at < ?)
        )`,
    )
        .bind(now, now, sessionId, staleBefore)
        .run();

    if (result.meta.changes !== 1) {
        return null;
    }

    return env.DB.prepare(
        `SELECT email, email_sent_at, email_status, license_key
      FROM sales
      WHERE stripe_checkout_session_id = ?`,
    )
        .bind(sessionId)
        .first<SaleRow>();
}

async function sendOwnershipEmail(sale: SaleRow, sessionId: string, env: Env): Promise<void> {
    const safeKey = escapeHtml(sale.license_key);
    const downloadUrl = new URL('/api/download?artifact=dmg', env.PUBLIC_SITE_URL).toString();
    const safeDownloadUrl = escapeHtml(downloadUrl);
    const sendResult = await env.EMAIL.send({
        from: {
            email: env.EMAIL_FROM,
            name: PRODUCT_NAME,
        },
        to: sale.email,
        replyTo: env.SUPPORT_EMAIL,
        subject: `Your ${PRODUCT_NAME} ownership key`,
        text: [
            `Thank you for purchasing ${PRODUCT_NAME}.`,
            '',
            `Your ownership key is: ${sale.license_key}`,
            '',
            'Keep this key somewhere safe. You will use it to prove ownership of the game.',
            '',
            `Download the free macOS demo: ${downloadUrl}`,
            '',
            `Need help? Reply to this email or contact ${env.SUPPORT_EMAIL}.`,
        ].join('\n'),
        html: `
      <div style="background:#08090c;color:#f4ead0;font-family:Georgia,serif;padding:32px">
        <div style="max-width:600px;margin:auto;border:1px solid #6f5732;background:#11151a;padding:32px">
          <p style="color:#d8aa55;text-transform:uppercase;letter-spacing:.12em">Purchase complete</p>
          <h1 style="color:#fff2cc">Take up the lantern.</h1>
          <p>Thank you for purchasing ${PRODUCT_NAME}.</p>
          <p>Your ownership key is:</p>
          <p style="padding:18px;background:#08090c;border:1px solid #d8aa55;color:#ffe0a0;font-family:monospace;font-size:20px;letter-spacing:.08em">${safeKey}</p>
          <p>Keep this key somewhere safe. You will use it to prove ownership of the game.</p>
          <p><a href="${safeDownloadUrl}" style="display:inline-block;padding:14px 18px;background:#d8aa55;color:#180b08;text-decoration:none;font-weight:bold">Download the free demo</a></p>
          <p style="color:#aeb6b0">Need help? Reply to this email or contact ${escapeHtml(env.SUPPORT_EMAIL)}.</p>
        </div>
      </div>
    `,
    });

    const now = new Date().toISOString();
    await env.DB.prepare(
        `UPDATE sales
      SET email_status = 'sent', email_message_id = ?, email_sent_at = ?, updated_at = ?
      WHERE stripe_checkout_session_id = ?`,
    )
        .bind(sendResult.messageId, now, now, sessionId)
        .run();
}

async function fulfillCheckout(sessionId: string, env: Env): Promise<void> {
    const stripe = stripeClient(env.STRIPE_SECRET_KEY);
    const session = await stripe.checkout.sessions.retrieve(sessionId);
    const expectedAmount = Number(env.GAME_PRICE_CENTS);

    if (session.payment_status !== 'paid') {
        throw new Error(`Checkout Session ${sessionId} is not paid`);
    }

    if (session.metadata?.product !== PRODUCT_SLUG) {
        throw new Error(`Checkout Session ${sessionId} has an unexpected product`);
    }

    if (session.amount_total !== expectedAmount || session.currency !== env.GAME_CURRENCY) {
        throw new Error(`Checkout Session ${sessionId} has an unexpected amount or currency`);
    }

    const email = session.customer_details?.email ?? session.customer_email;
    if (!email) {
        throw new Error(`Checkout Session ${sessionId} is missing a customer email`);
    }

    await insertSale(session, email, env);
    const claimedSale = await claimEmailDelivery(session.id, env);
    if (!claimedSale) {
        return;
    }

    try {
        await sendOwnershipEmail(claimedSale, session.id, env);
    } catch (error) {
        const now = new Date().toISOString();
        await env.DB.prepare(
            `UPDATE sales
        SET email_status = 'failed', updated_at = ?
        WHERE stripe_checkout_session_id = ? AND email_sent_at IS NULL`,
        )
            .bind(now, session.id)
            .run();
        throw error;
    }
}

async function handleStripeWebhook(request: Request, env: Env): Promise<Response> {
    const signature = request.headers.get('stripe-signature');
    if (!signature) {
        return new Response('Missing Stripe signature', { status: 400 });
    }

    let event: Stripe.Event;
    try {
        const payload = await request.text();
        event = await stripeClient(env.STRIPE_SECRET_KEY).webhooks.constructEventAsync(
            payload,
            signature,
            env.STRIPE_WEBHOOK_SECRET,
            undefined,
            Stripe.createSubtleCryptoProvider(),
        );
    } catch (error) {
        console.error(JSON.stringify({ event: 'stripe_webhook_rejected', error: String(error) }));
        return new Response('Invalid Stripe webhook', { status: 400 });
    }

    if (event.type === 'checkout.session.completed' || event.type === 'checkout.session.async_payment_succeeded') {
        const session = event.data.object;
        try {
            await fulfillCheckout(session.id, env);
        } catch (error) {
            console.error(
                JSON.stringify({
                    event: 'stripe_fulfillment_failed',
                    stripeEventId: event.id,
                    sessionId: session.id,
                    error: String(error),
                }),
            );
            return new Response('Fulfillment failed', { status: 500 });
        }
    }

    return json({ received: true });
}

async function purchaseStatus(request: Request, env: Env): Promise<Response> {
    const sessionId = new URL(request.url).searchParams.get('session_id');
    if (!sessionId?.startsWith('cs_')) {
        return json({ error: 'Invalid Checkout Session' }, { status: 400 });
    }

    const sale = await env.DB.prepare(
        `SELECT email, email_status, email_sent_at
      FROM sales
      WHERE stripe_checkout_session_id = ?`,
    )
        .bind(sessionId)
        .first<Pick<SaleRow, 'email' | 'email_status' | 'email_sent_at'>>();

    if (!sale) {
        return json({ status: 'processing' }, { status: 202 });
    }

    return json({
        status: sale.email_sent_at ? 'complete' : 'processing',
        email: maskEmail(sale.email),
        emailStatus: sale.email_status,
    });
}

async function downloadGame(request: Request, env: Env): Promise<Response> {
    const filename = `Pilgrim-of-the-Thorn-macOS-v${__PILGRIM_VERSION__}.dmg`;

    if (request.method === 'HEAD') {
        const object = await env.GAME_DOWNLOADS.head(filename);
        if (!object) {
            return json({ error: 'Download is not available yet.' }, { status: 404 });
        }

        const headers = downloadHeaders(object, filename);
        headers.set('content-length', String(object.size));
        return new Response(null, { headers });
    }

    const rangeHeader = request.headers.get('range');
    const requestedRange = rangeHeader?.match(/^bytes=(\d*)-(\d*)$/);
    const object = requestedRange
        ? await env.GAME_DOWNLOADS.get(filename, { range: request.headers })
        : await env.GAME_DOWNLOADS.get(filename);
    if (!object) {
        return json({ error: 'Download is not available yet.' }, { status: 404 });
    }

    const headers = downloadHeaders(object, filename);
    let status = 200;
    if (requestedRange) {
        const [, startValue, endValue] = requestedRange;
        let offset: number;
        let end: number;
        if (startValue) {
            offset = Number(startValue);
            end = endValue ? Math.min(Number(endValue), object.size - 1) : object.size - 1;
        } else {
            const suffixLength = Math.min(Number(endValue), object.size);
            offset = object.size - suffixLength;
            end = object.size - 1;
        }

        const length = end - offset + 1;
        status = 206;
        headers.set('content-range', `bytes ${offset}-${end}/${object.size}`);
        headers.set('content-length', String(length));
    } else {
        headers.set('content-length', String(object.size));
    }

    return new Response(object.body, { status, headers });
}

function downloadHeaders(object: R2Object, filename: string): Headers {
    const headers = new Headers();
    object.writeHttpMetadata(headers);
    headers.set('content-disposition', `attachment; filename="${filename}"`);
    headers.set('etag', object.httpEtag);
    headers.set('accept-ranges', 'bytes');
    headers.set('x-content-type-options', 'nosniff');
    headers.set('cache-control', 'no-store');
    return headers;
}

export default {
    async fetch(request, env): Promise<Response> {
        const url = new URL(request.url);

        try {
            if (url.pathname === '/api/checkout' && request.method === 'POST') {
                return await createCheckoutSession();
            }

            if (url.pathname === '/api/stripe/webhook' && request.method === 'POST') {
                return await handleStripeWebhook(request, env);
            }

            if (url.pathname === '/api/purchase-status' && request.method === 'GET') {
                return await purchaseStatus(request, env);
            }

            if (url.pathname === '/api/download' && (request.method === 'GET' || request.method === 'HEAD')) {
                return await downloadGame(request, env);
            }
        } catch (error) {
            console.error(
                JSON.stringify({
                    event: 'request_failed',
                    method: request.method,
                    pathname: url.pathname,
                    error: String(error),
                }),
            );
            return json({ error: 'Something went wrong. Please try again.' }, { status: 500 });
        }

        return new Response('Not found', {
            status: 404,
            headers: {
                'content-type': 'text/plain; charset=utf-8',
            },
        });
    },
} satisfies ExportedHandler<Env>;
