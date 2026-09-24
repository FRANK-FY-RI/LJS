CREATE TABLE IF NOT EXISTS submissions (
    submissions_id INTEGER PRIMARY KEY,
    user_id TEXT,
    code_id TEXT,
    verdict TEXT
);

CREATE TABLE IF NOT EXISTS codes (
    code_id TEXT PRIMARY KEY,
    code TEXT
);
