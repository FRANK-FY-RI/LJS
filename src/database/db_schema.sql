CREATE TABLE IF NOT EXISTS submissions (
    submissions_id TEXT,
    user_id TEXT,
    code_id TEXT,
    verdict TEXT
);

CREATE TABLE IF NOT EXISTS codes (
    code_id TEXT,
    code TEXT
);
