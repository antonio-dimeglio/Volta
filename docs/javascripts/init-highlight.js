// Initialize Volta syntax highlighting
document$.subscribe(function() {
  // Re-apply highlighting when page content changes (for MkDocs Material)
  document.querySelectorAll('code.language-volta, code.volta').forEach(function(block) {
    hljs.highlightElement(block);
  });
});

// Initial highlighting on page load
if (typeof hljs !== 'undefined') {
  hljs.highlightAll();
}
