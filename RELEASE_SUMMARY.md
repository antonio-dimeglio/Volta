# Release System Summary

## What I Set Up For You

### 1. GitHub Actions Release Workflow ✅

**File:** `.github/workflows/release.yml`

**Triggers:**
- **Automatic:** When you push a version tag (`v0.1.0`, `v1.0.0`, etc.)
- **Manual:** Click "Run workflow" in GitHub Actions UI

**Builds:**
- ✅ Linux (Ubuntu, x86_64) - `.tar.gz`
- ✅ macOS Intel (x86_64) - `.tar.gz`
- ✅ macOS Apple Silicon (ARM64) - `.tar.gz`
- ✅ Windows (x86_64) - `.zip`

**Output:**
- Creates GitHub Release automatically
- Uploads all 4 binaries
- Generates release notes
- Takes ~10-15 minutes

---

## How To Use

### Standard Release (Recommended)

```bash
# 1. Make sure you're on main and everything is committed
git checkout main
git pull

# 2. Create and push a version tag
git tag v0.1.0
git push origin v0.1.0

# 3. That's it! GitHub Actions does the rest
```

### What Happens Next

1. **GitHub detects the tag** and starts the release workflow
2. **4 parallel builds** start (Linux, macOS Intel, macOS ARM, Windows)
3. **Each build** compiles Volta and creates an archive
4. **Release is created** at `https://github.com/YOUR_USERNAME/volta/releases/tag/v0.1.0`
5. **All binaries** are attached to the release
6. **Release notes** are auto-generated

---

## Files I Created

| File | Purpose |
|------|---------|
| `.github/workflows/release.yml` | **Main release automation** - builds for 4 platforms |
| `RELEASE_GUIDE.md` | **Detailed documentation** - everything you need to know |
| `RELEASE_QUICK_REFERENCE.md` | **Quick commands** - cheat sheet for releases |
| `CHANGELOG.md` | **Version history** - track what changed in each version |

---

## Quick Commands

```bash
# Create release
git tag v0.1.0 && git push origin v0.1.0

# Delete tag (if mistake)
git push --delete origin v0.1.0 && git tag -d v0.1.0

# View releases
gh release list

# Download release
gh release download v0.1.0
```

---

## Testing the Release System

### Option 1: Use Manual Trigger (No Tag)

1. Go to: https://github.com/YOUR_USERNAME/volta/actions/workflows/release.yml
2. Click "Run workflow"
3. Enter version: `0.0.1-test`
4. Click "Run workflow"

This creates a test release without creating a git tag.

### Option 2: Use a Test Tag

```bash
git tag v0.0.1-test
git push origin v0.0.1-test
```

You can delete it later:
```bash
git push --delete origin v0.0.1-test
git tag -d v0.0.1-test
gh release delete v0.0.1-test --yes
```

---

## What's in Each Release Archive

### Linux & macOS (.tar.gz)
```
release/
├── volta          # The binary
├── README.md      # Documentation
└── LICENSE        # License file
```

### Windows (.zip)
```
release/
├── volta.exe      # The binary
├── README.md      # Documentation
└── LICENSE        # License file
```

---

## Version Numbering Guide

### Format: `vMAJOR.MINOR.PATCH`

**Examples:**
- `v0.1.0` - First alpha release
- `v0.2.0` - Second alpha (new features)
- `v0.2.1` - Bug fix for 0.2.0
- `v1.0.0` - First stable release (MVP)
- `v1.1.0` - New features, backward compatible
- `v2.0.0` - Breaking changes (self-hosted compiler)

**Pre-releases:**
- `v0.1.0-alpha.1` - Alpha version
- `v0.1.0-beta.1` - Beta version
- `v0.1.0-rc.1` - Release candidate

---

## Typical Release Workflow

### For Minor Releases (new features)

```bash
# 1. Develop on feature branch
git checkout -b feature-enums
# ... make changes ...
git commit -m "Implement enums"

# 2. Merge to main
git checkout main
git merge feature-enums
git push

# 3. Update CHANGELOG.md
vim CHANGELOG.md
git add CHANGELOG.md
git commit -m "Update changelog for v0.2.0"
git push

# 4. Tag and release
git tag v0.2.0
git push origin v0.2.0

# 5. Wait for build (~10 min)
# 6. Announce release!
```

### For Patch Releases (bug fixes)

```bash
# 1. Fix bug on main
git checkout main
# ... fix bug ...
git commit -m "Fix division by zero in VM"
git push

# 2. Tag patch release
git tag v0.1.1
git push origin v0.1.1

# Done!
```

---

## Monitoring Builds

### In Browser
https://github.com/YOUR_USERNAME/volta/actions

### With GitHub CLI
```bash
# Watch current run
gh run watch

# List recent runs
gh run list

# View specific run
gh run view 1234567890
```

---

## Common Issues & Solutions

### Build Fails on One Platform

**Symptom:** One platform build fails, others succeed

**Solution:**
1. Check the logs for that platform
2. Fix the issue in your workflow
3. Delete the release: `gh release delete v0.1.0 --yes`
4. Delete the tag: `git push --delete origin v0.1.0 && git tag -d v0.1.0`
5. Re-tag and push

### Missing libffi on Linux

**Error:** `fatal error: ffi.h: No such file or directory`

**Solution:** Add to workflow:
```yaml
sudo apt-get install -y libffi-dev
```
(Already included in your workflow!)

### macOS Homebrew Issues

**Error:** `Error: libffi not found`

**Solution:** Update Homebrew:
```yaml
brew update && brew install libffi
```
(Already included in your workflow!)

### Windows MSYS2 Package Issues

**Error:** Package not found

**Solution:** Check package names:
```yaml
mingw-w64-x86_64-gcc
mingw-w64-x86_64-make
mingw-w64-x86_64-libffi
```
(Already configured in your workflow!)

---

## Release Checklist

Use this before each release:

```markdown
## Pre-Release
- [ ] All tests pass: `make test`
- [ ] On main branch: `git branch --show-current`
- [ ] Working tree clean: `git status`
- [ ] CHANGELOG.md updated
- [ ] Version number decided
- [ ] Examples still work

## Release
- [ ] Tag created: `git tag v0.1.0`
- [ ] Tag pushed: `git push origin v0.1.0`
- [ ] Builds monitored: Check GitHub Actions
- [ ] All 4 platforms succeeded

## Post-Release
- [ ] Binaries downloaded and tested
- [ ] Release notes accurate
- [ ] README.md updated with new version
- [ ] Announcement made (if applicable)
```

---

## Advanced: Customizing Release Notes

### Option 1: Edit Workflow File

Edit `.github/workflows/release.yml`, section "Create release notes":

```yaml
- name: Create release notes
  run: |
    cat > release_notes.md << 'EOF'
    # Your custom release notes here

    ## What's New
    - Feature 1
    - Feature 2

    ## Bug Fixes
    - Fix 1
    - Fix 2
    EOF
```

### Option 2: Use CHANGELOG.md

The workflow can read from CHANGELOG.md:

```yaml
body_path: CHANGELOG.md
```

Or extract a specific version section with a script.

---

## Security: Checksums (Optional Enhancement)

To add SHA256 checksums to releases, add this step before "Create GitHub Release":

```yaml
- name: Generate checksums
  run: |
    cd artifacts
    find . -type f \( -name "*.tar.gz" -o -name "*.zip" \) -exec sha256sum {} \; > SHA256SUMS
    cat SHA256SUMS

- name: Create GitHub Release
  uses: softprops/action-gh-release@v2
  with:
    # ... existing config ...
    files: |
      artifacts/**/*.tar.gz
      artifacts/**/*.zip
      artifacts/SHA256SUMS  # Add this line
```

Users can then verify:
```bash
sha256sum -c SHA256SUMS
```

---

## Example: Your First Release

Let's do a test release right now!

```bash
# 1. Checkout main
git checkout main
git pull

# 2. Make sure it builds locally
make clean
make test

# 3. Update CHANGELOG (optional for test)
echo "## [0.1.0-alpha.1] - $(date +%Y-%m-%d)" >> CHANGELOG.md
echo "- Test release" >> CHANGELOG.md
git add CHANGELOG.md
git commit -m "Prepare test release"
git push

# 4. Create test tag
git tag v0.1.0-alpha.1
git push origin v0.1.0-alpha.1

# 5. Monitor
# Go to: https://github.com/YOUR_USERNAME/volta/actions
# Wait ~10-15 minutes

# 6. Check result
# Go to: https://github.com/YOUR_USERNAME/volta/releases

# 7. Download and test
gh release download v0.1.0-alpha.1
tar -xzf volta-0.1.0-alpha.1-linux-x86_64.tar.gz
cd release
./volta --version

# Success! 🎉
```

---

## When to Release

### Alpha Releases (0.x.x)
- Early development
- Features incomplete
- Breaking changes expected
- For testing and feedback

### Beta Releases (0.x.x-beta.x)
- Most features complete
- Stabilizing
- Breaking changes possible but rare
- Wider testing

### Release Candidates (x.x.x-rc.x)
- Feature-complete
- Only critical bug fixes
- No breaking changes
- Final testing before stable

### Stable Releases (x.x.x)
- Production-ready
- Semantic versioning guarantees
- No breaking changes in minor versions
- For end users

---

## Your Release Roadmap

Based on your MVP evaluation:

```
v0.1.0-alpha.1    - Current state (Nov 2025)
                    Basic features, no enums/structs

v0.2.0-alpha.1    - Enums + pattern matching (Jan 2026)
                    Sprint 1-4 complete

v0.3.0-alpha.1    - Structs + tuples (Mar 2026)
                    Sprint 5-7 complete

v0.4.0-beta.1     - Standard library (Apr 2026)
                    Sprint 8 complete

v1.0.0-rc.1       - Release candidate (May 2026)
                    All features done, testing

v1.0.0            - MVP RELEASE! (Jun 2026)
                    Production ready

v1.5.0            - Modules + imports (Sep 2026)

v2.0.0            - Self-hosted! (Dec 2026)
                    Compiler written in Volta

v3.0.0            - Scientific computing (2027)
                    Original vision complete
```

---

## Summary

✅ **You now have:**
- Automated builds for 4 platforms
- One-command releases (`git tag v0.1.0 && git push origin v0.1.0`)
- Professional release workflow
- Comprehensive documentation

🚀 **Next steps:**
1. Test the system with `v0.1.0-alpha.1`
2. Use it for all future releases
3. Update CHANGELOG.md with each version
4. Announce releases to your users

📚 **Resources:**
- Full guide: [RELEASE_GUIDE.md](RELEASE_GUIDE.md)
- Quick commands: [RELEASE_QUICK_REFERENCE.md](RELEASE_QUICK_REFERENCE.md)
- Version history: [CHANGELOG.md](CHANGELOG.md)

---

**You're ready to release! Just push a tag and watch the magic happen.** ✨
