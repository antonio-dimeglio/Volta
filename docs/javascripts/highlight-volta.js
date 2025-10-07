// Volta language syntax highlighting for Highlight.js
(function() {
  if (typeof hljs !== 'undefined') {
    hljs.registerLanguage('volta', function(hljs) {
      return {
        name: 'Volta',
        aliases: ['vlt'],
        keywords: {
          keyword: 'fn return if else while for in match struct mut import break continue as',
          built_in: 'Some None',
          literal: 'true false',
          type: 'int float bool str void Array Option Matrix'
        },
        contains: [
          // Line comments
          hljs.COMMENT('#', '$', {
            relevance: 0
          }),
          // Block comments
          {
            className: 'comment',
            begin: '#\\[',
            end: '\\]#',
            relevance: 10
          },
          // Documentation comments
          {
            className: 'comment',
            begin: '#\\[doc\\]',
            end: '#\\[/doc\\]',
            relevance: 10,
            contains: [
              {
                className: 'doctag',
                begin: '@(param|returns|field|example|throws)\\b'
              }
            ]
          },
          // Strings
          {
            className: 'string',
            begin: '"',
            end: '"',
            illegal: '\\n',
            contains: [hljs.BACKSLASH_ESCAPE]
          },
          // Numbers
          {
            className: 'number',
            variants: [
              { begin: '\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?' }, // float/int
              { begin: '\\b0x[0-9a-fA-F]+' } // hex
            ],
            relevance: 0
          },
          // Function definitions
          {
            className: 'function',
            beginKeywords: 'fn',
            end: '\\{|=|->',
            excludeEnd: true,
            contains: [
              {
                className: 'title',
                begin: hljs.IDENT_RE,
                relevance: 0
              },
              {
                className: 'params',
                begin: '\\(',
                end: '\\)',
                keywords: {
                  keyword: 'mut',
                  type: 'int float bool str void Array Option Matrix'
                },
                contains: [
                  hljs.COMMENT('#', '$'),
                  {
                    className: 'type',
                    begin: ':\\s*',
                    end: '[,\\)]',
                    excludeEnd: true,
                    keywords: 'int float bool str void Array Option Matrix'
                  }
                ]
              }
            ]
          },
          // Struct definitions
          {
            className: 'class',
            beginKeywords: 'struct',
            end: '\\{',
            excludeEnd: true,
            contains: [
              {
                className: 'title',
                begin: hljs.IDENT_RE
              }
            ]
          },
          // Type annotations
          {
            begin: ':\\s*',
            end: '[,\\)\\{\\}=]',
            excludeEnd: true,
            keywords: {
              type: 'int float bool str void Array Option Matrix',
              keyword: 'mut fn'
            }
          },
          // Operators
          {
            className: 'operator',
            begin: '\\+|\\-|\\*|/|%|\\*\\*|==|!=|<=|>=|<|>|=|\\+=|\\-=|\\*=|/=|:=|\\.\\.|\\.\\.=|->|=>'
          },
          // Generic types
          {
            begin: '\\b(Array|Option|Matrix)\\[',
            end: '\\]',
            keywords: {
              type: 'Array Option Matrix int float bool str'
            },
            contains: ['self']
          }
        ]
      };
    });
  }
})();
