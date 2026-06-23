CREATE TABLE IF NOT EXISTS sales (
  stripe_checkout_session_id TEXT PRIMARY KEY,
  stripe_payment_intent_id TEXT,
  stripe_customer_id TEXT,
  email TEXT NOT NULL,
  amount_total INTEGER NOT NULL,
  currency TEXT NOT NULL,
  license_key TEXT NOT NULL UNIQUE,
  payment_status TEXT NOT NULL CHECK (payment_status IN ('paid', 'refunded')),
  email_status TEXT NOT NULL CHECK (email_status IN ('pending', 'sending', 'sent', 'failed')),
  email_message_id TEXT,
  email_attempted_at TEXT,
  email_sent_at TEXT,
  created_at TEXT NOT NULL,
  updated_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS sales_email_idx ON sales(email);
CREATE INDEX IF NOT EXISTS sales_payment_intent_idx ON sales(stripe_payment_intent_id);
