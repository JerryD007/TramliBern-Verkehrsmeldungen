#!/usr/bin/env bash
set -euo pipefail
# Literal destination, no branch input or arbitrary file staging.
test_branch='test/tramlibern-3.2-verified'
test "$(git branch --show-current)" = "$test_branch"
python3 ci/check_feed_publication.py reports/alerts-v2.json
mkdir -p data
cp reports/alerts-v2.json data/alerts-v2.json
git config user.name 'github-actions[bot]'
git config user.email '41898282+github-actions[bot]@users.noreply.github.com'
git add -- data/alerts-v2.json
if ! git diff --cached --quiet; then
  git commit -m 'Refresh verified 3.2 test feed'
  # No rebase, merge, force push or retry of data from an obsolete code revision.
  git push origin HEAD:refs/heads/test/tramlibern-3.2-verified
fi
