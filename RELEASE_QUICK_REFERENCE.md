# Release Quick Reference

## TL;DR

```bash
# Create and push a version tag
git tag v0.1.0
git push origin v0.1.0

# Wait 10-15 minutes
# Check: https://github.com/YOUR_USERNAME/volta/actions

# Done! Binaries are at:
# https://github.com/YOUR_USERNAME/volta/releases
```

---

## Commands

### Create Release
```bash
git tag v0.1.0 && git push origin v0.1.0
```

### Re-release (if build fails)
```bash
# Delete tag
git push --delete origin v0.1.0
git tag -d v0.1.0

# Fix issues, then re-tag
git tag v0.1.0
git push origin v0.1.0
```

### Manual Release (no tag)
Go to: https://github.com/YOUR_USERNAME/volta/actions/workflows/release.yml
Click "Run workflow", enter version, click "Run workflow"

---

## What Gets Built

| Platform | File | Runner |
|----------|------|--------|
| Linux x86_64 | `volta-VERSION-linux-x86_64.tar.gz` | Ubuntu Latest |
| macOS Intel | `volta-VERSION-macos-x86_64.tar.gz` | macOS 13 |
| macOS ARM | `volta-VERSION-macos-arm64.tar.gz` | macOS 14 |
| Windows x86_64 | `volta-VERSION-windows-x86_64.zip` | Windows Latest |

---

## Version Numbers

```
v0.1.0       First alpha
v0.2.0       Second alpha
v1.0.0       MVP release
v1.1.0       Feature update
v1.1.1       Bug fix
```

**Pre-releases:**
```
v0.1.0-alpha.1
v0.1.0-beta.1
v0.1.0-rc.1
```

---

## Workflow Triggers

| Trigger | When | Use Case |
|---------|------|----------|
| `push: tags: v*` | You push a version tag | **Normal releases** ✅ |
| `workflow_dispatch` | Manual trigger in UI | Testing, hotfixes |

---

## Checklist

Before releasing:
- [ ] Tests pass: `make test`
- [ ] On main branch: `git checkout main`
- [ ] Changes committed: `git status`
- [ ] CHANGELOG.md updated
- [ ] Tag ready: `git tag v0.1.0`

To release:
- [ ] Push tag: `git push origin v0.1.0`
- [ ] Monitor: Check GitHub Actions
- [ ] Verify: Download and test binaries
- [ ] Announce: Update README, tweet, etc.

---

## Troubleshooting

**Build failed?**
- Check logs in GitHub Actions
- Fix issue
- Delete tag and re-release

**Wrong version number?**
```bash
git push --delete origin v0.1.0
git tag -d v0.1.0
git tag v0.2.0
git push origin v0.2.0
```

**Need to update a release?**
- Delete release in GitHub UI
- Delete and re-push tag
- Or manually edit release and upload new binaries

---

## GitHub CLI Commands

```bash
# Install: https://cli.github.com

# List releases
gh release list

# View release
gh release view v0.1.0

# Download release
gh release download v0.1.0

# Delete release
gh release delete v0.1.0 --yes

# Delete tag
git push --delete origin v0.1.0
```

---

## Example Release Session

```bash
# 1. Update changelog
echo "## [0.1.0] - $(date +%Y-%m-%d)" >> CHANGELOG.md
echo "- Initial release" >> CHANGELOG.md
git add CHANGELOG.md
git commit -m "Update changelog for v0.1.0"

# 2. Tag and push
git tag v0.1.0
git push origin v0.1.0

# 3. Monitor (optional)
gh run watch

# 4. Verify
gh release view v0.1.0

# Done! 🚀
```

---

## URLs to Bookmark

- Actions: `https://github.com/YOUR_USERNAME/volta/actions`
- Releases: `https://github.com/YOUR_USERNAME/volta/releases`
- Workflow: `https://github.com/YOUR_USERNAME/volta/actions/workflows/release.yml`

---

## Common Scenarios

### First Release
```bash
git tag v0.1.0-alpha.1 -m "First alpha release"
git push origin v0.1.0-alpha.1
```

### Hot Fix
```bash
git checkout main
git cherry-pick <commit>
git tag v1.0.1
git push origin v1.0.1
```

### Pre-release
```bash
git tag v1.0.0-rc.1 -m "Release candidate 1"
git push origin v1.0.0-rc.1
```

### Stable Release
```bash
git tag v1.0.0 -m "Stable release"
git push origin v1.0.0
```

---

## Tips

✅ **DO:**
- Test locally before releasing
- Update CHANGELOG.md
- Use semantic versioning
- Create annotated tags (`git tag -a`)
- Release from `main` branch

❌ **DON'T:**
- Release from feature branches
- Skip testing
- Forget to update docs
- Use same version twice
- Push tags before ready

---

## Need Help?

- 📖 Full guide: [RELEASE_GUIDE.md](RELEASE_GUIDE.md)
- 🐛 Issues: https://github.com/YOUR_USERNAME/volta/issues
- 💬 Discussions: https://github.com/YOUR_USERNAME/volta/discussions

---

**Remember:** Just push a tag and GitHub Actions does the rest! 🎉
