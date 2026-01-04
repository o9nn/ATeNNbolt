# Scripts Directory

This directory contains utility scripts for the Bolt C++ project.

## validate-roadmap.js

Validates that `DEVO-GENESIS.md` is properly formatted for the `generate-next-steps.yml` GitHub Actions workflow.

### Usage

```bash
node scripts/validate-roadmap.js
```

### What it checks

- Presence of "## Next Development Steps" section
- Proper timeline format: `1. **Timeline Name**:`
- Task format with checkboxes: `- [ ] Task description` or `- [x] Completed task`
- Generates summary of tasks and expected GitHub issue creation
- Shows timeline label names that will be created

### Example output

```
✅ Found "Next Development Steps" section
✅ Found 4 timeline sections:
   1. Immediate (Week 1-2): 6 tasks (6 pending, 0 complete)
   2. Short-term (Month 1): 8 tasks (8 pending, 0 complete)
   ...

🚀 GitHub Actions will create 30 issues for pending tasks
```

This script helps ensure that the roadmap format is compatible with the automated issue generation workflow.