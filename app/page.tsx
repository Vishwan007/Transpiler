'use client';

import React, { useState, useCallback, useMemo, useEffect } from 'react';
import { SQLToNoSQLTranscompiler, type CompilationResult } from '@/lib/transcompiler';
import { Copy, Play, RotateCcw, Download } from 'lucide-react';

const examples = {
  selectSimple: 'SELECT * FROM users WHERE age > 25 LIMIT 10;',
  selectAgg: 'SELECT user_id, COUNT(*) as cnt, SUM(amount) as total FROM orders WHERE amount > 100 GROUP BY user_id ORDER BY total DESC LIMIT 10;',
  insert: "INSERT INTO users VALUES ('John', 'john@email.com', 30);",
  update: 'UPDATE orders SET amount = 150 WHERE user_id = 5;',
  delete: 'DELETE FROM orders WHERE id = 10;',
  createTable: 'CREATE TABLE customers (id INT, name VARCHAR(100), email TEXT, active BOOLEAN);',
  dropTable: 'DROP TABLE products;',
  truncateTable: 'TRUNCATE TABLE orders;',
  alterTable: 'ALTER TABLE users ADD phone VARCHAR(20);',
  showTables: 'SHOW TABLES;',
  describeTable: 'DESCRIBE users;',
};

export default function TranscompilerApp() {
  const [sql, setSql] = useState(examples.selectSimple);
  const [result, setResult] = useState<CompilationResult | null>(null);
  const [copied, setCopied] = useState(false);
  const [activeTab, setActiveTab] = useState<'output' | 'errors'>('output');
  const [schema, setSchema] = useState<Record<string, string[]>>({});

  // Reuse the same compiler instance to maintain session state (like created tables)
  const compiler = useMemo(() => new SQLToNoSQLTranscompiler(), []);

  // Initialize schema on mount
  useEffect(() => {
    setSchema(compiler.getSchema());
  }, [compiler]);

  const compileSQL = useCallback(() => {
    const compilationResult = compiler.compile(sql);
    setResult(compilationResult);
    setSchema(compiler.getSchema());

    if (compilationResult.errors.length > 0) {
      setActiveTab('errors');
    } else {
      setActiveTab('output');
    }
  }, [sql, compiler]);

  const copyToClipboard = () => {
    if (result?.mongodb) {
      navigator.clipboard.writeText(result.mongodb);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    }
  };

  const loadExample = (key: keyof typeof examples) => {
    setSql(examples[key]);
    setResult(null);
  };

  const downloadResult = () => {
    if (!result?.mongodb) return;
    const element = document.createElement('a');
    element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(result.mongodb));
    element.setAttribute('download', 'query.mongodb.js');
    element.style.display = 'none';
    document.body.appendChild(element);
    element.click();
    document.body.removeChild(element);
  };

  return (
    <div className="min-h-screen bg-slate-950 text-slate-50">
      {/* Header */}
      <header className="border-b border-slate-800 bg-gradient-to-b from-slate-900 to-slate-950 py-6 px-6 sticky top-0 z-10">
        <div className="max-w-7xl mx-auto">
          <div className="flex items-center justify-between mb-2">
            <div>
              <h1 className="text-3xl font-bold text-balance">SQL to NoSQL Transcompiler</h1>
              <p className="text-slate-400 text-sm mt-1">Convert SQL queries to MongoDB aggregation pipelines</p>
            </div>
            <div className="text-sm text-slate-500 bg-slate-900/50 px-3 py-1 rounded border border-slate-800">
              4-Phase Compiler
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <div className="max-w-7xl mx-auto p-6">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* SQL Input Panel */}
          <div className="flex flex-col gap-4">
            <div className="bg-slate-900 rounded-lg border border-slate-800 overflow-hidden flex flex-col h-96">
              <div className="bg-slate-800 px-4 py-3 border-b border-slate-700 flex items-center justify-between">
                <h2 className="font-semibold text-slate-100">SQL Query</h2>
                <div className="text-xs text-slate-400">Input</div>
              </div>
              <textarea
                value={sql}
                onChange={(e) => setSql(e.target.value)}
                className="flex-1 bg-slate-950 text-slate-100 font-mono text-sm p-4 resize-none border-none focus:outline-none focus:ring-0"
                placeholder="SELECT * FROM users WHERE age > 25;"
                spellCheck="false"
              />
            </div>

            {/* Action Buttons */}
            <div className="flex gap-3">
              <button
                onClick={compileSQL}
                className="flex-1 bg-emerald-600 hover:bg-emerald-700 text-white font-medium py-2 px-4 rounded-lg flex items-center justify-center gap-2 transition-colors"
              >
                <Play size={18} />
                Compile
              </button>
              <button
                onClick={() => setSql('')}
                className="bg-slate-800 hover:bg-slate-700 text-slate-100 font-medium py-2 px-4 rounded-lg flex items-center justify-center gap-2 transition-colors"
              >
                <RotateCcw size={18} />
                Clear
              </button>
            </div>

            {/* Examples */}
            <div>
              <h3 className="text-sm font-semibold text-slate-300 mb-2">Quick Examples</h3>
              <div className="grid grid-cols-2 gap-2">
                <button
                  onClick={() => loadExample('selectSimple')}
                  className="text-left text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 p-2 rounded border border-slate-700 transition-colors"
                >
                  SELECT Simple
                </button>
                <button
                  onClick={() => loadExample('selectAgg')}
                  className="text-left text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 p-2 rounded border border-slate-700 transition-colors"
                >
                  SELECT + Aggregate
                </button>
                <button
                  onClick={() => loadExample('insert')}
                  className="text-left text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 p-2 rounded border border-slate-700 transition-colors"
                >
                  INSERT
                </button>
                <button
                  onClick={() => loadExample('update')}
                  className="text-left text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 p-2 rounded border border-slate-700 transition-colors"
                >
                  UPDATE
                </button>
                <button
                  onClick={() => loadExample('createTable')}
                  className="text-left text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 p-2 rounded border border-slate-700 transition-colors"
                >
                  CREATE TABLE
                </button>
                <button
                  onClick={() => loadExample('alterTable')}
                  className="text-left text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 p-2 rounded border border-slate-700 transition-colors"
                >
                  ALTER TABLE
                </button>
                <button
                  onClick={() => loadExample('dropTable')}
                  className="text-left text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 p-2 rounded border border-slate-700 transition-colors"
                >
                  DROP TABLE
                </button>
                <button
                  onClick={() => loadExample('showTables')}
                  className="text-left text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 p-2 rounded border border-slate-700 transition-colors"
                >
                  SHOW TABLES
                </button>
              </div>
            </div>

            {/* Database Explorer */}
            <div className="bg-slate-900 rounded-lg border border-slate-800 overflow-hidden flex flex-col flex-1 min-h-64">
              <div className="bg-slate-800 px-4 py-3 border-b border-slate-700 flex items-center justify-between">
                <h2 className="font-semibold text-slate-100">Database Explorer</h2>
                <div className="text-xs text-slate-400">Schema</div>
              </div>
              <div className="p-4 overflow-y-auto max-h-64 space-y-3">
                {Object.keys(schema).length > 0 ? (
                  Object.entries(schema).map(([table, cols]) => (
                    <div key={table} className="bg-slate-950 border border-slate-800 rounded p-2">
                      <div className="text-xs font-bold text-emerald-400 mb-1">TABLE: {table}</div>
                      <div className="flex flex-wrap gap-1">
                        {cols.map((col) => (
                          <span key={col} className="text-[10px] bg-slate-900 text-slate-400 px-1.5 py-0.5 rounded border border-slate-800">
                            {col}
                          </span>
                        ))}
                      </div>
                    </div>
                  ))
                ) : (
                  <p className="text-xs text-slate-500 italic">Run a query to see current schema</p>
                )}
              </div>
            </div>
          </div>

          {/* Output Panel */}
          <div className="flex flex-col gap-4">
            {result ? (
              <>
                {/* Compilation Result */}
                <div className="bg-slate-900 rounded-lg border border-slate-800 overflow-hidden flex flex-col">
                  <div className="bg-slate-800 px-4 py-3 border-b border-slate-700 flex items-center justify-between">
                    <h2 className="font-semibold text-slate-100">MongoDB Output</h2>
                    <div className={`text-xs px-2 py-1 rounded ${result.success ? 'bg-emerald-900 text-emerald-300' : 'bg-red-900 text-red-300'}`}>
                      {result.success ? '✓ Success' : '✗ Errors'}
                    </div>
                  </div>
                  <pre className="flex-1 bg-slate-950 text-slate-100 font-mono text-xs p-4 overflow-auto max-h-96">
                    {result.mongodb || 'No output'}
                  </pre>
                </div>

                {/* Tabs */}
                <div className="flex gap-2 border-b border-slate-800">
                  <button
                    onClick={() => setActiveTab('output')}
                    className={`px-4 py-2 text-sm font-medium transition-colors ${
                      activeTab === 'output'
                        ? 'text-emerald-400 border-b-2 border-emerald-400'
                        : 'text-slate-400 hover:text-slate-300'
                    }`}
                  >
                    Compilation ({result.phases.lexical.length + result.phases.syntax.length + result.phases.semantic.length})
                  </button>
                  <button
                    onClick={() => setActiveTab('errors')}
                    className={`px-4 py-2 text-sm font-medium transition-colors ${
                      activeTab === 'errors'
                        ? 'text-emerald-400 border-b-2 border-emerald-400'
                        : 'text-slate-400 hover:text-slate-300'
                    }`}
                  >
                    Errors ({result.errors.length})
                  </button>
                </div>

                {/* Tab Content */}
                <div className="bg-slate-900 rounded-lg border border-slate-800 p-4 max-h-64 overflow-y-auto">
                  {activeTab === 'output' ? (
                    <div className="space-y-4 text-sm">
                      <div>
                        <h4 className="font-semibold text-slate-200 mb-2">Lexical Analysis</h4>
                        {result.phases.lexical.length === 0 ? (
                          <p className="text-emerald-400">✓ No errors</p>
                        ) : (
                          <ul className="space-y-1">
                            {result.phases.lexical.map((err, i) => (
                              <li key={i} className="text-red-400">
                                {err.message} (Line {err.line}, Col {err.column})
                              </li>
                            ))}
                          </ul>
                        )}
                      </div>
                      <div>
                        <h4 className="font-semibold text-slate-200 mb-2">Syntax Analysis</h4>
                        {result.phases.syntax.length === 0 ? (
                          <p className="text-emerald-400">✓ No errors</p>
                        ) : (
                          <ul className="space-y-1">
                            {result.phases.syntax.map((err, i) => (
                              <li key={i} className="text-red-400">
                                {err.message}
                              </li>
                            ))}
                          </ul>
                        )}
                      </div>
                      <div>
                        <h4 className="font-semibold text-slate-200 mb-2">Semantic Analysis</h4>
                        {result.phases.semantic.length === 0 ? (
                          <p className="text-emerald-400">✓ No errors</p>
                        ) : (
                          <ul className="space-y-1">
                            {result.phases.semantic.map((err, i) => (
                              <li key={i} className="text-red-400">
                                {err.message}
                              </li>
                            ))}
                          </ul>
                        )}
                      </div>
                    </div>
                  ) : (
                    <div className="space-y-2 text-sm">
                      {result.errors.length === 0 ? (
                        <p className="text-emerald-400">✓ No errors found</p>
                      ) : (
                        result.errors.map((err, i) => (
                          <div key={i} className="bg-red-950 border border-red-800 rounded p-2">
                            <div className="font-semibold text-red-300">{err.level}</div>
                            <div className="text-red-200 text-xs mt-1">{err.message}</div>
                            {err.context && <div className="text-red-400 text-xs mt-1 italic">Context: {err.context}</div>}
                          </div>
                        ))
                      )}
                    </div>
                  )}
                </div>

                {/* Download Button */}
                {result.success && (
                  <button
                    onClick={copyToClipboard}
                    className={`w-full py-2 px-4 rounded-lg font-medium flex items-center justify-center gap-2 transition-colors ${
                      copied
                        ? 'bg-emerald-600 text-white'
                        : 'bg-slate-800 hover:bg-slate-700 text-slate-100'
                    }`}
                  >
                    <Copy size={18} />
                    {copied ? 'Copied!' : 'Copy to Clipboard'}
                  </button>
                )}
              </>
            ) : (
              <div className="bg-slate-900 rounded-lg border border-slate-800 border-dashed flex items-center justify-center h-96 text-slate-400">
                <div className="text-center">
                  <div className="text-4xl mb-2">📝</div>
                  <p>Enter SQL and click Compile to see the MongoDB output</p>
                </div>
              </div>
            )}
          </div>
        </div>

        {/* Features Section */}
        <div className="mt-12 grid grid-cols-1 md:grid-cols-4 gap-4">
          <div className="bg-slate-900 border border-slate-800 rounded-lg p-4">
            <div className="text-2xl mb-2">🔍</div>
            <h3 className="font-semibold text-slate-100 mb-1">Lexical Analysis</h3>
            <p className="text-xs text-slate-400">Tokenizes SQL into language components</p>
          </div>
          <div className="bg-slate-900 border border-slate-800 rounded-lg p-4">
            <div className="text-2xl mb-2">📐</div>
            <h3 className="font-semibold text-slate-100 mb-1">Syntax Analysis</h3>
            <p className="text-xs text-slate-400">Parses tokens into AST structures</p>
          </div>
          <div className="bg-slate-900 border border-slate-800 rounded-lg p-4">
            <div className="text-2xl mb-2">✓</div>
            <h3 className="font-semibold text-slate-100 mb-1">Semantic Analysis</h3>
            <p className="text-xs text-slate-400">Validates tables, columns & types</p>
          </div>
          <div className="bg-slate-900 border border-slate-800 rounded-lg p-4">
            <div className="text-2xl mb-2">⚙️</div>
            <h3 className="font-semibold text-slate-100 mb-1">Code Generation</h3>
            <p className="text-xs text-slate-400">Generates MongoDB queries</p>
          </div>
        </div>
      </div>
    </div>
  );
}
