# Pilgrim of the Thorn Web

Cloudflare Worker + Vite storefront for the free macOS demo and paid edition of Pilgrim of the Thorn.

## Purchase flow

The site uses Stripe-hosted Checkout, so card numbers never pass through this app.

1. `POST /api/checkout` creates a one-time $19.95 Checkout Session.
2. Stripe redirects the buyer to its hosted payment page.
3. Stripe calls `POST /api/stripe/webhook` after payment.
4. The Worker verifies the webhook and retrieves the Checkout Session from Stripe.
5. A sale and cryptographically random ownership key are stored in D1.
6. Cloudflare Email Service sends the key to the buyer.

Stripe remains the payment ledger. D1 is the app's fulfillment and license ledger.

## One-time setup

### 1. Stripe

Create or activate a Stripe account, then copy the test-mode secret key.

```sh
cp .env.example .env
```

Put these values in `.env`:

```dotenv
STRIPE_SECRET_KEY=sk_test_...
STRIPE_WEBHOOK_SECRET=whsec_...
```

For production, save them as Worker secrets:

```sh
npx wrangler secret put STRIPE_SECRET_KEY
npx wrangler secret put STRIPE_WEBHOOK_SECRET
```

In Stripe Workbench, create a webhook destination:

- URL: `https://YOUR_DOMAIN/api/stripe/webhook`
- Events: `checkout.session.completed` and `checkout.session.async_payment_succeeded`

Copy that destination's signing secret into `STRIPE_WEBHOOK_SECRET`.

The checkout currently accepts Stripe's default payment methods and does not enable Stripe Tax. Decide where the game is sold and configure tax collection before a broad production launch.

### 2. D1

The `DB` binding is configured for automatic provisioning on deploy. To create the database explicitly instead:

```sh
npx wrangler d1 create pilgrim-of-the-thorn-sales
```

If Wrangler returns a `database_id`, add it to the `DB` entry in `wrangler.jsonc`. Apply the schema:

```sh
npm run db:migrate:local
npm run db:migrate:remote
```

### 3. Transactional email

Replace `example.com` in `wrangler.jsonc` with a domain on the same Cloudflare account:

```jsonc
"EMAIL_FROM": "orders@yourdomain.com",
"SUPPORT_EMAIL": "support@yourdomain.com",
"PUBLIC_SITE_URL": "https://yourdomain.com"
```

Also update `allowed_sender_addresses`, then onboard the domain:

```sh
npx wrangler email sending enable yourdomain.com
npx wrangler email sending dns get yourdomain.com
```

Cloudflare will configure the required SPF and DKIM records. Add DMARC if the domain does not already have it.

### 4. Generate binding types

```sh
npm run types
```

## Local webhook testing

Install the Stripe CLI, start the site, and forward test events:

```sh
npm run db:migrate:local
npm run dev
stripe listen --forward-to http://127.0.0.1:5173/api/stripe/webhook
```

Copy the CLI's temporary `whsec_...` value into `.env`, then use Stripe's test card `4242 4242 4242 4242` with any future expiration date and any three-digit CVC.

Email sending requires an onboarded domain and a remote email binding. Until then, checkout and webhook verification can be tested, but fulfillment email will fail and Stripe will retry the webhook.

## Commands

```sh
npm install
npm run types
npm run db:migrate:local
npm run dev
npm run build
npm run deploy
```

`npm run deploy` rebuilds and packages the game, copies the versioned DMG and game art into `public/`, builds the Vite app, and deploys it with Wrangler.

The ownership key is issued and stored, but the game does not validate it yet. Add an activation screen or ownership-key verification endpoint before relying on the key for access control.
