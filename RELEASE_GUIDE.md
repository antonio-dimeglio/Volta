# Release Guide

How to create release builds for Volta.

## Quick Start

### Option 1: Tag-based Release (Recommended)

This is the standard way to release software:

```bash
# 1. Make sure main branch is ready
git checkout main
git pull

# 2. Create and push a version tag
git tag v0.1.0
git push origin v0.1.0
```

That's it! GitHub Actions will automatically:
- Build for Linux (x86_64)
- Build for macOS Intel (x86_64)
- Build for macOS Apple Silicon (arm64)
- Build for Windows (x86_64)
- Create a GitHub release with all binaries
- Generate release notes

### Option 2: Manual Trigger

If you want to test the release process without creating a tag:

1. Go to: https://github.com/YOUR_USERNAME/volta/actions/workflows/release.yml
2. Click "Run workflow"
3. Enter a version number (e.g., `0.1.0-test`)
4. Click "Run workflow"

This will create a release with the version you specified.

---

## Release Workflow Details

### What Gets Built

Each platform gets a standalone binary:

- **Linux**: `volta-VERSION-linux-x86_64.tar.gz`
  - Built on Ubuntu with GCC
  - Statically linked where possible

- **macOS Intel**: `volta-VERSION-macos-x86_64.tar.gz`
  - Built on macOS 13 (Intel runner)
  - Universal binary for Intel Macs

- **macOS ARM**: `volta-VERSION-macos-arm64.tar.gz`
  - Built on macOS 14 (Apple Silicon runner)
  - Native ARM64 binary

- **Windows**: `volta-VERSION-windows-x86_64.zip`
  - Built with MinGW-w64 on MSYS2
  - Includes volta.exe

### Build Time

Expect ~10-15 minutes total:
- Each platform builds in parallel
- Linux: ~3-4 min
- macOS: ~4-5 min each
- Windows: ~5-6 min

---

## Version Numbering

We use [Semantic Versioning](https://semver.org/):

```
MAJOR.MINOR.PATCH
```

- **MAJOR**: Breaking changes (e.g., `1.0.0` → `2.0.0`)
- **MINOR**: New features, backward compatible (e.g., `1.0.0` → `1.1.0`)
- **PATCH**: Bug fixes (e.g., `1.0.0` → `1.0.1`)

### Pre-releases

Use suffixes for pre-releases:
- Alpha: `v0.1.0-alpha.1`
- Beta: `v0.1.0-beta.1`
- Release Candidate: `v0.1.0-rc.1`

---

## Step-by-Step Release Process

### 1. Prepare for Release

```bash
# Update version in code (if you have a version constant)
# vim src/main.cpp

# Update CHANGELOG.md
vim CHANGELOG.md

# Commit changes
git add -A
git commit -m "Prepare for release v0.1.0"
git push
```

### 2. Create Release Tag

```bash
# Create annotated tag (recommended)
git tag -a v0.1.0 -m "Release v0.1.0: Initial MVP"

# Push tag to trigger release
git push origin v0.1.0
```

### 3. Monitor Build

```bash
# Open in browser:
# https://github.com/YOUR_USERNAME/volta/actions
```

Or use GitHub CLI:
```bash
gh run watch
```

### 4. Verify Release

Once complete, check:
1. All 4 platform builds succeeded ✅
2. Release is created: `https://github.com/YOUR_USERNAME/volta/releases`
3. All binaries are attached
4. Release notes look correct

### 5. Test Downloads

Download and test each platform:

**Linux/macOS:**
```bash
# Download
wget https://github.com/YOUR_USERNAME/volta/releases/download/v0.1.0/volta-0.1.0-linux-x86_64.tar.gz

# Extract
tar -xzf volta-0.1.0-linux-x86_64.tar.gz
cd release

# Test
./volta --version
./volta examples/hello.vlt
```

**Windows:**
```powershell
# Download and extract the .zip
# Then test:
.\volta.exe --version
.\volta.exe examples\hello.vlt
```

---

## Troubleshooting

### Build Failed on One Platform

1. Check the logs: Click on the failed job in GitHub Actions
2. Common issues:
   - **Linux**: Missing libffi (`sudo apt-get install libffi-dev`)
   - **macOS**: Homebrew issues (`brew update && brew install libffi`)
   - **Windows**: MSYS2 package issues (check msys2 setup)

3. Fix the issue in your workflow file
4. Delete the failed release (if created)
5. Delete the tag: `git tag -d v0.1.0 && git push --delete origin v0.1.0`
6. Re-create the tag after fixing

### Release Already Exists

If you need to re-release:

```bash
# Delete remote tag
git push --delete origin v0.1.0

# Delete local tag
git tag -d v0.1.0

# Delete GitHub release (or use UI)
gh release delete v0.1.0

# Re-create tag
git tag v0.1.0
git push origin v0.1.0
```

### Binary Doesn't Run

Check platform compatibility:
- **Linux**: Requires glibc 2.31+ (Ubuntu 20.04+)
- **macOS**: Requires macOS 10.15+ (Intel) or 11.0+ (ARM)
- **Windows**: Requires Windows 10+

---

## Release Checklist

Before each release:

- [ ] All tests pass (`make test`)
- [ ] Code is merged to `main`
- [ ] CHANGELOG.md is updated
- [ ] Version number is chosen
- [ ] Tag is created and pushed
- [ ] Build completes successfully
- [ ] All 4 binaries are tested
- [ ] Release notes are accurate
- [ ] Documentation is up to date

---

## Advanced: Custom Release Notes

To customize release notes, edit `.github/workflows/release.yml`:

```yaml
- name: Create release notes
  run: |
    cat > release_notes.md << 'EOF'
    # Your custom release notes here

    ## Breaking Changes
    - List breaking changes

    ## New Features
    - List new features

    ## Bug Fixes
    - List bug fixes
    EOF
```

Or maintain a `RELEASE_NOTES.md` file and use it:

```yaml
body_path: RELEASE_NOTES.md
```

---

## CI/CD Best Practices

### Don't Release from Feature Branches

Always release from `main`:
```bash
# Bad
git checkout feature-branch
git tag v0.1.0  # Don't do this!

# Good
git checkout main
git merge feature-branch
git tag v0.1.0
```

### Use Draft Releases for Testing

Modify the workflow to create draft releases:

```yaml
draft: true  # Change to true
prerelease: false
```

Then manually publish after testing.

### Add Checksums (Optional)

For security, generate SHA256 checksums:

```yaml
- name: Generate checksums
  run: |
    cd artifacts
    sha256sum */*.tar.gz */*.zip > SHA256SUMS
```

---

## GitHub CLI Shortcuts

Install: https://cli.github.com/

```bash
# View releases
gh release list

# View specific release
gh release view v0.1.0

# Download release assets
gh release download v0.1.0

# Delete release
gh release delete v0.1.0

# Create manual release
gh release create v0.1.0 --title "Version 0.1.0" --notes "Release notes"
```

---

## Example: First Release

```bash
# On main branch, everything committed
git checkout main
git pull

# Create CHANGELOG.md if you don't have one
cat > CHANGELOG.md << 'EOF'
# Changelog

## [0.1.0] - 2025-10-07

### Added
- Initial MVP release
- Basic language features (variables, functions, control flow)
- For loops with range operators
- Print builtin
- Working VM with GC

### Known Issues
- Pattern matching not yet implemented
- Structs not yet implemented
- Standard library minimal

EOF

git add CHANGELOG.md
git commit -m "Add CHANGELOG.md for v0.1.0"
git push

# Create and push tag
git tag -a v0.1.0 -m "Release v0.1.0: Initial MVP"
git push origin v0.1.0

# Wait for build (~10 minutes)
# Check: https://github.com/YOUR_USERNAME/volta/actions

# Done! Release is at:
# https://github.com/YOUR_USERNAME/volta/releases/tag/v0.1.0
```

---

## Summary

**To release:**
```bash
git tag v0.1.0 && git push origin v0.1.0
```

**That's it!** GitHub Actions handles the rest:
- ✅ Builds for 4 platforms
- ✅ Creates release
- ✅ Uploads binaries
- ✅ Generates notes

**Next release:**
```bash
git tag v0.2.0 && git push origin v0.2.0
```

Simple and automated! 🚀
