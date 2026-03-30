// SQL to NoSQL Transcompiler Engine
// 4-Phase Compiler: Lexer → Parser → Semantic Analyzer → ICG Generator

export type ErrorLevel = 'LEXICAL_ERROR' | 'SYNTAX_ERROR' | 'SEMANTIC_ERROR' | 'TRANSLATION_ERROR';

export interface CompilerError {
  level: ErrorLevel;
  message: string;
  line: number;
  column: number;
  context: string;
}

export interface CompilationResult {
  success: boolean;
  mongodb: string;
  errors: CompilerError[];
  warnings: string[];
  phases: {
    lexical: CompilerError[];
    syntax: CompilerError[];
    semantic: CompilerError[];
  };
}

// Token types
enum TokenType {
  KEYWORD,
  IDENTIFIER,
  NUMBER,
  STRING,
  OPERATOR,
  PUNCTUATION,
  EOF,
}

interface Token {
  type: TokenType;
  value: string;
  line: number;
  column: number;
}

// Lexer Phase
class Lexer {
  private source: string;
  private pos: number = 0;
  private line: number = 1;
  private column: number = 1;
  private tokens: Token[] = [];
  private errors: CompilerError[] = [];

  private keywords = new Set([
    'SELECT', 'INSERT', 'UPDATE', 'DELETE', 'FROM', 'WHERE', 'AND', 'OR',
    'NOT', 'IN', 'LIKE', 'BETWEEN', 'IS', 'NULL', 'GROUP', 'BY', 'HAVING',
    'ORDER', 'ASC', 'DESC', 'LIMIT', 'DISTINCT', 'COUNT', 'SUM', 'AVG',
    'MIN', 'MAX', 'JOIN', 'INNER', 'LEFT', 'RIGHT', 'FULL', 'OUTER', 'ON',
    'VALUES', 'SET', 'AS', 'CASE', 'WHEN', 'THEN', 'ELSE', 'END', 'CAST',
    'TRANSACTION', 'BEGIN', 'COMMIT', 'ROLLBACK', 'CREATE', 'TABLE', 'VARCHAR', 'INT', 'TEXT', 'DATE', 'BOOLEAN',
    'PRIMARY', 'KEY', 'NULL',
    'SHOW', 'TABLES', 'DESCRIBE', 'DESC', 'DROP', 'TRUNCATE', 'ALTER', 'ADD', 'RENAME', 'TO',
  ]);

  constructor(source: string) {
    this.source = source.toUpperCase();
  }

  tokenize(): { tokens: Token[]; errors: CompilerError[] } {
    while (this.pos < this.source.length) {
      this.skipWhitespace();
      if (this.pos >= this.source.length) break;

      const char = this.source[this.pos];

      if (/[a-zA-Z_]/.test(char)) {
        this.readIdentifierOrKeyword();
      } else if (/[0-9]/.test(char)) {
        this.readNumber();
      } else if (char === "'") {
        this.readString();
      } else if (/[+\-*/<>=!]/.test(char)) {
        this.readOperator();
      } else if (/[(),;.]/.test(char)) {
        this.tokens.push({
          type: TokenType.PUNCTUATION,
          value: char,
          line: this.line,
          column: this.column,
        });
        this.advance();
      } else {
        this.errors.push({
          level: 'LEXICAL_ERROR',
          message: `Invalid character: '${char}'`,
          line: this.line,
          column: this.column,
          context: this.getContext(),
        });
        this.advance();
      }
    }

    this.tokens.push({ type: TokenType.EOF, value: '', line: this.line, column: this.column });
    return { tokens: this.tokens, errors: this.errors };
  }

  private readIdentifierOrKeyword() {
    const start = this.pos;
    while (this.pos < this.source.length && /[a-zA-Z0-9_]/.test(this.source[this.pos])) {
      this.advance();
    }
    const value = this.source.slice(start, this.pos);
    const type = this.keywords.has(value) ? TokenType.KEYWORD : TokenType.IDENTIFIER;
    this.tokens.push({ type, value, line: this.line, column: this.column - value.length });
  }

  private readNumber() {
    const start = this.pos;
    while (this.pos < this.source.length && /[0-9.]/.test(this.source[this.pos])) {
      this.advance();
    }
    const value = this.source.slice(start, this.pos);
    this.tokens.push({ type: TokenType.NUMBER, value, line: this.line, column: this.column - value.length });
  }

  private readString() {
    const startCol = this.column;
    const startLine = this.line;
    this.advance(); // Skip opening quote
    const start = this.pos;

    while (this.pos < this.source.length && this.source[this.pos] !== "'") {
      if (this.source[this.pos] === '\\') this.advance();
      this.advance();
    }

    if (this.pos >= this.source.length) {
      this.errors.push({
        level: 'LEXICAL_ERROR',
        message: 'Unterminated string literal',
        line: startLine,
        column: startCol,
        context: this.getContext(),
      });
      return;
    }

    const value = this.source.slice(start, this.pos);
    this.tokens.push({ type: TokenType.STRING, value, line: startLine, column: startCol });
    this.advance(); // Skip closing quote
  }

  private readOperator() {
    const start = this.pos;
    const ch1 = this.source[this.pos];
    const ch2 = this.source[this.pos + 1];

    if ((ch1 === '<' && ch2 === '=') || (ch1 === '>' && ch2 === '=') || (ch1 === '<' && ch2 === '>') || (ch1 === '!' && ch2 === '=')) {
      this.tokens.push({ type: TokenType.OPERATOR, value: ch1 + ch2, line: this.line, column: this.column });
      this.advance();
      this.advance();
    } else {
      this.tokens.push({ type: TokenType.OPERATOR, value: ch1, line: this.line, column: this.column });
      this.advance();
    }
  }

  private skipWhitespace() {
    while (this.pos < this.source.length && /\s/.test(this.source[this.pos])) {
      if (this.source[this.pos] === '\n') {
        this.line++;
        this.column = 1;
      } else {
        this.column++;
      }
      this.pos++;
    }
  }

  private advance() {
    if (this.source[this.pos] === '\n') {
      this.line++;
      this.column = 1;
    } else {
      this.column++;
    }
    this.pos++;
  }

  private getContext(): string {
    const start = Math.max(0, this.pos - 20);
    const end = Math.min(this.source.length, this.pos + 20);
    return this.source.slice(start, end);
  }
}

// Parser Phase
interface ColumnDefinition {
  name: string;
  type: string;
  isNullable: boolean;
  isPrimaryKey: boolean;
}

interface SQLQuery {
  type: 'SELECT' | 'INSERT' | 'UPDATE' | 'DELETE' | 'CREATE' | 'DROP' | 'TRUNCATE' | 'ALTER' | 'SHOW' | 'DESCRIBE' | 'TRANSACTION';
  table: string;
  columns?: string[];
  columnDefinitions?: ColumnDefinition[];
  values?: any[];
  where?: WhereClause;
  groupBy?: string[];
  having?: WhereClause;
  orderBy?: OrderByClause[];
  limit?: number;
  joins?: JoinClause[];
  aggregates?: AggregateFunction[];
  alterOperation?: 'ADD' | 'DROP' | 'RENAME';
  newColumnName?: string;
  transactionType?: 'BEGIN' | 'COMMIT' | 'ROLLBACK';
}

interface WhereClause {
  field: string;
  operator: string;
  value: any;
  logic?: 'AND' | 'OR';
  conditions?: WhereClause[];
}

interface OrderByClause {
  field: string;
  direction: 'ASC' | 'DESC';
}

interface JoinClause {
  type: 'INNER' | 'LEFT' | 'RIGHT';
  table: string;
  on: { left: string; right: string };
}

interface AggregateFunction {
  name: string;
  field: string;
  alias: string;
}

class Parser {
  private tokens: Token[];
  private pos: number = 0;
  private errors: CompilerError[] = [];

  constructor(tokens: Token[]) {
    this.tokens = tokens;
  }

  parse(): { query: SQLQuery | null; errors: CompilerError[] } {
    const query = this.parseQuery();
    return { query, errors: this.errors };
  }

  private parseQuery(): SQLQuery | null {
    if (this.isKeyword('SELECT')) {
      return this.parseSelect();
    } else if (this.isKeyword('INSERT')) {
      return this.parseInsert();
    } else if (this.isKeyword('UPDATE')) {
      return this.parseUpdate();
    } else if (this.isKeyword('DELETE')) {
      return this.parseDelete();
    } else if (this.isKeyword('CREATE')) {
      return this.parseCreate();
    } else if (this.isKeyword('DROP')) {
      return this.parseDrop();
    } else if (this.isKeyword('TRUNCATE')) {
      return this.parseTruncate();
    } else if (this.isKeyword('ALTER')) {
      return this.parseAlter();
    } else if (this.isKeyword('SHOW')) {
      return this.parseShow();
    } else if (this.isKeyword('DESCRIBE') || this.isKeyword('DESC')) {
      return this.parseDescribe();
    } else if (this.isKeyword('BEGIN') || this.isKeyword('COMMIT') || this.isKeyword('ROLLBACK') || this.isKeyword('TRANSACTION')) {
      return this.parseTransaction();
    } else {
      this.error('Expected SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, TRUNCATE, ALTER, SHOW, DESCRIBE, or TRANSACTION');
      return null;
    }
  }

  private parseDrop(): SQLQuery | null {
    this.expect('DROP');
    this.expect('TABLE');
    const table = this.expect('IDENTIFIER')?.value || '';
    return { type: 'DROP', table };
  }

  private parseTruncate(): SQLQuery | null {
    this.expect('TRUNCATE');
    this.expect('TABLE');
    const table = this.expect('IDENTIFIER')?.value || '';
    return { type: 'TRUNCATE', table };
  }

  private parseAlter(): SQLQuery | null {
    this.expect('ALTER');
    this.expect('TABLE');
    const table = this.expect('IDENTIFIER')?.value || '';
    let alterOperation: 'ADD' | 'DROP' | 'RENAME' | undefined;
    let newColumnName: string | undefined;
    let columnDefinitions: ColumnDefinition[] | undefined;

    if (this.isKeyword('ADD')) {
      this.advance();
      alterOperation = 'ADD';
      const colName = this.expect('IDENTIFIER')?.value || '';
      const type = this.current()?.value || 'TEXT';
      if (this.current()?.type === TokenType.KEYWORD) this.advance();
      columnDefinitions = [{ name: colName, type, isNullable: true, isPrimaryKey: false }];
    } else if (this.isKeyword('DROP')) {
      this.advance();
      this.expect('COLUMN');
      alterOperation = 'DROP';
      const colName = this.expect('IDENTIFIER')?.value || '';
      return { type: 'ALTER', table, alterOperation, columns: [colName] };
    } else if (this.isKeyword('RENAME')) {
      this.advance();
      this.expect('TO');
      alterOperation = 'RENAME';
      newColumnName = this.expect('IDENTIFIER')?.value || '';
    }

    return { type: 'ALTER', table, alterOperation, newColumnName, columnDefinitions };
  }

  private parseShow(): SQLQuery | null {
    this.expect('SHOW');
    this.expect('TABLES');
    return { type: 'SHOW', table: '' };
  }

  private parseDescribe(): SQLQuery | null {
    if (this.isKeyword('DESCRIBE')) this.advance();
    else this.expect('DESC');
    const table = this.expect('IDENTIFIER')?.value || '';
    return { type: 'DESCRIBE', table };
  }

  private parseTransaction(): SQLQuery | null {
    let transactionType: 'BEGIN' | 'COMMIT' | 'ROLLBACK' = 'BEGIN';
    if (this.isKeyword('BEGIN')) {
      this.advance();
      if (this.isKeyword('TRANSACTION')) this.advance();
      transactionType = 'BEGIN';
    } else if (this.isKeyword('COMMIT')) {
      this.advance();
      transactionType = 'COMMIT';
    } else if (this.isKeyword('ROLLBACK')) {
      this.advance();
      transactionType = 'ROLLBACK';
    } else if (this.isKeyword('TRANSACTION')) {
      this.advance();
      transactionType = 'BEGIN'; // Default to begin
    }
    return { type: 'TRANSACTION', table: '', transactionType };
  }

  private parseCreate(): SQLQuery | null {
    this.expect('CREATE');
    this.expect('TABLE');
    const tableToken = this.expect('IDENTIFIER');
    if (!tableToken) return null;
    const table = tableToken.value;

    this.expect('(');
    const columnDefinitions: ColumnDefinition[] = [];

    while (this.pos < this.tokens.length && !this.check(')')) {
      const colNameToken = this.expect('IDENTIFIER');
      if (!colNameToken) return null;

      const typeToken = this.current();
      let colType = 'TEXT';
      if (typeToken && typeToken.type === TokenType.KEYWORD) {
        colType = typeToken.value;
        this.advance();
        // Handle VARCHAR(255) etc
        if (this.check('(')) {
          this.advance();
          if (!this.expect('NUMBER')) return null;
          if (!this.expect(')')) return null;
        }
      }

      // Handle constraints like PRIMARY KEY, NOT NULL
      let isPrimaryKey = false;
      let isNullable = true;

      while (this.pos < this.tokens.length && !this.check(',', ')')) {
        if (this.isKeyword('PRIMARY')) {
          this.advance();
          this.expect('KEY');
          isPrimaryKey = true;
          isNullable = false; // Primary keys cannot be null
        } else if (this.isKeyword('NOT')) {
          this.advance();
          this.expect('NULL');
          isNullable = false;
        } else if (this.isKeyword('NULL')) {
          this.advance();
          isNullable = true;
        } else {
          // Skip other unknown column constraints for now
          this.advance();
        }
      }

      columnDefinitions.push({ name: colNameToken.value, type: colType, isNullable, isPrimaryKey });

      if (this.check(',')) {
        this.advance();
      } else if (!this.check(')')) {
        this.error(`Expected , or ) in column definition, got ${this.current()?.value}`);
        return null;
      }
    }

    if (!this.expect(')')) return null;
    return { type: 'CREATE', table, columnDefinitions };
  }

  private parseSelect(): SQLQuery {
    this.expect('SELECT');
    const columns = this.parseColumnList();
    this.expect('FROM');
    const table = this.expect('IDENTIFIER')!.value;

    const query: SQLQuery = { type: 'SELECT', table, columns };

    while (this.pos < this.tokens.length && this.tokens[this.pos].type !== TokenType.EOF) {
      if (this.isKeyword('WHERE')) {
        this.advance();
        query.where = this.parseWhereClause();
      } else if (this.isKeyword('GROUP')) {
        this.advance();
        this.expect('BY');
        query.groupBy = this.parseFieldList();
      } else if (this.isKeyword('ORDER')) {
        this.advance();
        this.expect('BY');
        query.orderBy = this.parseOrderBy();
      } else if (this.isKeyword('LIMIT')) {
        this.advance();
        const num = this.expect('NUMBER');
        query.limit = num ? parseInt(num.value) : 0;
      } else if (this.isKeyword('JOIN') || this.isKeyword('INNER') || this.isKeyword('LEFT') || this.isKeyword('RIGHT')) {
        query.joins = query.joins || [];
        query.joins.push(this.parseJoin());
      } else {
        break;
      }
    }

    return query;
  }

  private parseInsert(): SQLQuery {
    this.expect('INSERT');
    this.expect('INTO');
    const table = this.expect('IDENTIFIER')!.value;
    this.expect('VALUES');
    this.expect('(');
    const values = this.parseValues();
    this.expect(')');
    return { type: 'INSERT', table, values };
  }

  private parseUpdate(): SQLQuery {
    this.expect('UPDATE');
    const table = this.expect('IDENTIFIER')!.value;
    this.expect('SET');
    // Simplified: just parse field = value pairs
    const setClause: Record<string, any> = {};
    while (!this.isKeyword('WHERE') && this.pos < this.tokens.length) {
      const field = this.expect('IDENTIFIER')!.value;
      this.expect('=');
      const value = this.parseValue();
      setClause[field] = value;
      if (this.check('(', ',')) this.advance();
    }

    const query: SQLQuery = { type: 'UPDATE', table };
    if (this.isKeyword('WHERE')) {
      this.advance();
      query.where = this.parseWhereClause();
    }
    return query;
  }

  private parseDelete(): SQLQuery {
    this.expect('DELETE');
    this.expect('FROM');
    const table = this.expect('IDENTIFIER')!.value;
    const query: SQLQuery = { type: 'DELETE', table };

    if (this.isKeyword('WHERE')) {
      this.advance();
      query.where = this.parseWhereClause();
    }
    return query;
  }

  private parseColumnList(): string[] {
    const columns: string[] = [];
    if (this.check('*')) {
      this.advance();
      return ['*'];
    }
    columns.push(this.expect('IDENTIFIER')!.value);
    while (this.check('(', ',')) {
      this.advance();
      if (this.isKeyword('COUNT') || this.isKeyword('SUM') || this.isKeyword('AVG') || this.isKeyword('MIN') || this.isKeyword('MAX')) {
        const aggFunc = this.current()!.value;
        this.advance();
        this.expect('(');
        const field = this.expect('IDENTIFIER')!.value;
        this.expect(')');
        if (this.isKeyword('AS')) {
          this.advance();
          const alias = this.expect('IDENTIFIER')!.value;
          columns.push(`${aggFunc}(${field}) AS ${alias}`);
        } else {
          columns.push(`${aggFunc}(${field})`);
        }
      } else {
        columns.push(this.expect('IDENTIFIER')!.value);
      }
    }
    return columns;
  }

  private parseFieldList(): string[] {
    const fields: string[] = [];
    fields.push(this.expect('IDENTIFIER')!.value);
    while (this.check('(', ',')) {
      this.advance();
      fields.push(this.expect('IDENTIFIER')!.value);
    }
    return fields;
  }

  private parseWhereClause(): WhereClause {
    const field = this.expect('IDENTIFIER')!.value;
    const operator = this.expect('OPERATOR')!.value;
    const value = this.parseValue();

    const clause: WhereClause = { field, operator, value };

    if (this.isKeyword('AND') || this.isKeyword('OR')) {
      clause.logic = this.current()!.value as 'AND' | 'OR';
      this.advance();
      clause.conditions = [this.parseWhereClause()];
    }

    return clause;
  }

  private parseOrderBy(): OrderByClause[] {
    const clauses: OrderByClause[] = [];
    clauses.push({
      field: this.expect('IDENTIFIER')!.value,
      direction: this.isKeyword('ASC') || this.isKeyword('DESC') ? (this.advance() as any, this.tokens[this.pos - 1].value as any) : 'ASC',
    });

    while (this.check('(', ',')) {
      this.advance();
      clauses.push({
        field: this.expect('IDENTIFIER')!.value,
        direction: this.isKeyword('ASC') || this.isKeyword('DESC') ? (this.advance() as any, this.tokens[this.pos - 1].value as any) : 'ASC',
      });
    }

    return clauses;
  }

  private parseJoin(): JoinClause {
    let type: 'INNER' | 'LEFT' | 'RIGHT' = 'INNER';
    if (this.isKeyword('LEFT') || this.isKeyword('RIGHT')) {
      type = this.current()!.value as 'LEFT' | 'RIGHT';
      this.advance();
    }
    this.expect('JOIN');
    const table = this.expect('IDENTIFIER')!.value;
    this.expect('ON');
    const left = this.expect('IDENTIFIER')!.value;
    this.expect('=');
    const right = this.expect('IDENTIFIER')!.value;

    return { type, table, on: { left, right } };
  }

  private parseValues(): any[] {
    const values: any[] = [];
    values.push(this.parseValue());
    while (this.check('(', ',')) {
      this.advance();
      values.push(this.parseValue());
    }
    return values;
  }

  private parseValue(): any {
    const token = this.current();
    if (!token) return null;

    if (token.type === TokenType.NUMBER) {
      this.advance();
      return parseInt(token.value);
    } else if (token.type === TokenType.STRING) {
      this.advance();
      return token.value;
    } else if (token.type === TokenType.IDENTIFIER) {
      this.advance();
      return token.value;
    }
    return null;
  }

  private isKeyword(word: string): boolean {
    const token = this.current();
    return token?.type === TokenType.KEYWORD && token?.value === word;
  }

  private check(...chars: string[]): boolean {
    const token = this.current();
    return token ? chars.includes(token.value) : false;
  }

  private expect(type: string): Token | null {
    const token = this.current();
    if (!token) {
      this.error(`Unexpected end of input, expected ${type}`);
      return null;
    }

    if (type === 'IDENTIFIER' && token.type !== TokenType.IDENTIFIER) {
      this.error(`Expected identifier, got ${token.value}`);
      return null;
    } else if (type === 'NUMBER' && token.type !== TokenType.NUMBER) {
      this.error(`Expected number, got ${token.value}`);
      return null;
    } else if (type === 'OPERATOR' && token.type !== TokenType.OPERATOR) {
      this.error(`Expected operator, got ${token.value}`);
      return null;
    } else if (type === 'STRING' && token.type !== TokenType.STRING) {
      this.error(`Expected string, got ${token.value}`);
      return null;
    } else if (token.value !== type && type !== 'IDENTIFIER' && type !== 'NUMBER' && type !== 'OPERATOR' && type !== 'STRING') {
      this.error(`Expected ${type}, got ${token.value}`);
      return null;
    }

    this.advance();
    return token;
  }

  private current(): Token | null {
    return this.pos < this.tokens.length ? this.tokens[this.pos] : null;
  }

  private advance(): Token | null {
    return this.tokens[this.pos++] || null;
  }

  private error(message: string) {
    const token = this.current();
    this.errors.push({
      level: 'SYNTAX_ERROR',
      message,
      line: token?.line || 0,
      column: token?.column || 0,
      context: token?.value || '',
    });
  }
}

// Semantic Analyzer Phase
class SemanticAnalyzer {
  private errors: CompilerError[] = [];
  private tables = new Set(['USERS', 'ORDERS', 'PRODUCTS', 'CUSTOMERS']);
  private tableColumns: Record<string, string[]> = {
    USERS: ['ID', 'NAME', 'EMAIL', 'AGE', 'CREATED_AT'],
    ORDERS: ['ID', 'USER_ID', 'PRODUCT_ID', 'AMOUNT', 'CREATED_AT'],
    PRODUCTS: ['ID', 'NAME', 'PRICE', 'STOCK'],
    CUSTOMERS: ['ID', 'NAME', 'PHONE', 'ADDRESS'],
  };

  analyze(query: SQLQuery): CompilerError[] {
    this.errors = []; // Clear errors for each analysis

    if (query.type === 'SHOW') return this.errors;
    if (query.type === 'TRANSACTION') return this.errors;

    if (query.type === 'CREATE') {
      if (this.tables.has(query.table)) {
        this.errors.push({
          level: 'SEMANTIC_ERROR',
          message: `Table '${query.table}' already exists`,
          line: 0,
          column: 0,
          context: `Cannot create existing table`,
        });
      } else if (query.columnDefinitions) {
        // "Create" the table in memory for the current session
        this.tables.add(query.table);
        this.tableColumns[query.table] = query.columnDefinitions.map((col) => col.name);
      }
      return this.errors;
    }

    if (query.type === 'DROP') {
      if (!this.tables.has(query.table)) {
        this.errors.push({
          level: 'SEMANTIC_ERROR',
          message: `Table '${query.table}' does not exist`,
          line: 0,
          column: 0,
          context: `Cannot drop non-existent table`,
        });
      } else {
        this.tables.delete(query.table);
        delete this.tableColumns[query.table];
      }
      return this.errors;
    }

    if (query.type === 'ALTER') {
      if (!this.tables.has(query.table)) {
        this.errors.push({
          level: 'SEMANTIC_ERROR',
          message: `Table '${query.table}' does not exist`,
          line: 0,
          column: 0,
          context: `Cannot alter non-existent table`,
        });
      } else if (query.alterOperation === 'ADD' && query.columnDefinitions) {
        this.tableColumns[query.table].push(query.columnDefinitions[0].name);
      } else if (query.alterOperation === 'DROP' && query.columns) {
        this.tableColumns[query.table] = this.tableColumns[query.table].filter((c) => c !== query.columns![0]);
      } else if (query.alterOperation === 'RENAME' && query.newColumnName) {
        const oldName = query.table;
        const newName = query.newColumnName;
        this.tables.delete(oldName);
        this.tables.add(newName);
        this.tableColumns[newName] = this.tableColumns[oldName];
        delete this.tableColumns[oldName];
      }
      return this.errors;
    }

    if (!this.tables.has(query.table)) {
      this.errors.push({
        level: 'SEMANTIC_ERROR',
        message: `Table '${query.table}' does not exist`,
        line: 0,
        column: 0,
        context: `Available tables: ${Array.from(this.tables).join(', ')}`,
      });
    }

    if (query.type === 'DESCRIBE') return this.errors;
    if (query.type === 'TRUNCATE') return this.errors;

    if (query.columns && query.columns[0] !== '*') {
      const tableColumns = this.tableColumns[query.table] || [];
      for (const col of query.columns) {
        const cleanCol = col.split('(')[0].split(' ')[0].trim();
        if (!tableColumns.includes(cleanCol) && !col.includes('COUNT') && !col.includes('SUM') && !col.includes('AVG')) {
          this.errors.push({
            level: 'SEMANTIC_ERROR',
            message: `Column '${cleanCol}' does not exist in table '${query.table}'`,
            line: 0,
            column: 0,
            context: `Available columns: ${tableColumns.join(', ')}`,
          });
        }
      }
    }

    if (query.where) {
      this.validateWhereClause(query.where, query.table);
    }

    return this.errors;
  }

  getSchema(): Record<string, string[]> {
    return { ...this.tableColumns };
  }

  private validateWhereClause(where: WhereClause, table: string) {
    const tableColumns = this.tableColumns[table] || [];
    if (!tableColumns.includes(where.field)) {
      this.errors.push({
        level: 'SEMANTIC_ERROR',
        message: `Column '${where.field}' does not exist in WHERE clause`,
        line: 0,
        column: 0,
        context: `Available columns: ${tableColumns.join(', ')}`,
      });
    }
  }
}

// ICG Generator Phase
class ICGGenerator {
  generate(query: SQLQuery): string {
    switch (query.type) {
      case 'SELECT':
        return this.generateSelect(query);
      case 'INSERT':
        return this.generateInsert(query);
      case 'UPDATE':
        return this.generateUpdate(query);
      case 'DELETE':
        return this.generateDelete(query);
      case 'CREATE':
        return this.generateCreate(query);
      case 'DROP':
        return `db.${query.table}.drop()`;
      case 'TRUNCATE':
        return `db.${query.table}.deleteMany({})`;
      case 'SHOW':
        return `db.getCollectionNames()`;
      case 'DESCRIBE':
        return `db.getCollectionInfos({ name: "${query.table}" })`;
      case 'TRANSACTION':
        const tType = query.transactionType || 'BEGIN';
        return `// Transaction: ${tType}\nsession.${tType.toLowerCase()}Transaction()`;
      case 'ALTER':
        return this.generateAlter(query);
      default:
        return '';
    }
  }

  private generateAlter(query: SQLQuery): string {
    if (query.alterOperation === 'ADD' && query.columnDefinitions) {
      const col = query.columnDefinitions[0];
      return `db.${query.table}.updateMany({}, { $set: { "${col.name}": null } })`;
    } else if (query.alterOperation === 'DROP' && query.columns) {
      return `db.${query.table}.updateMany({}, { $unset: { "${query.columns[0]}": "" } })`;
    } else if (query.alterOperation === 'RENAME') {
      return `db.${query.table}.renameCollection("${query.newColumnName}")`;
    }
    return '';
  }

  private generateCreate(query: SQLQuery): string {
    const required = query.columnDefinitions?.filter((col) => !col.isNullable).map((col) => col.name) || [];

    const validator: Record<string, any> = {
      $jsonSchema: {
        bsonType: 'object',
        required,
        properties: {},
      },
    };

    query.columnDefinitions?.forEach((col) => {
      let bsonType = 'string';
      const typeMap: Record<string, string> = {
        INT: 'int',
        INTEGER: 'int',
        VARCHAR: 'string',
        TEXT: 'string',
        BOOLEAN: 'bool',
        DATE: 'date',
        DOUBLE: 'double',
        FLOAT: 'double',
      };
      bsonType = typeMap[col.type] || 'string';
      validator.$jsonSchema.properties[col.name] = { bsonType };
    });

    return `db.createCollection("${query.table}", ${JSON.stringify({ validator }, null, 2)})`;
  }

  private generateSelect(query: SQLQuery): string {
    const pipeline: Record<string, any>[] = [];

    // Match stage
    if (query.where) {
      pipeline.push({ $match: this.buildMatch(query.where) });
    }

    // Group stage
    if (query.groupBy) {
      const groupId: Record<string, any> = {};
      query.groupBy.forEach((field) => {
        groupId[field] = `$${field}`;
      });

      const groupStage: Record<string, any> = { _id: groupId };

      if (query.columns) {
        query.columns.forEach((col) => {
          if (col.includes('COUNT(*)')) {
            groupStage.count = { $sum: 1 };
          } else if (col.includes('SUM(')) {
            const field = col.match(/SUM\((\w+)\)/)?.[1];
            if (field) groupStage.total = { $sum: `$${field}` };
          } else if (col.includes('AVG(')) {
            const field = col.match(/AVG\((\w+)\)/)?.[1];
            if (field) groupStage.avg = { $avg: `$${field}` };
          }
        });
      }

      pipeline.push({ $group: groupStage });
    }

    // Sort stage
    if (query.orderBy) {
      const sort: Record<string, number> = {};
      query.orderBy.forEach((order) => {
        sort[order.field] = order.direction === 'ASC' ? 1 : -1;
      });
      pipeline.push({ $sort: sort });
    }

    // Limit stage
    if (query.limit) {
      pipeline.push({ $limit: query.limit });
    }

    // Project stage
    if (query.columns && query.columns[0] !== '*') {
      const project: Record<string, number> = {};
      query.columns.forEach((col) => {
        const field = col.split(' ')[0].split('(')[0];
        project[field] = 1;
      });
      pipeline.push({ $project: project });
    }

    return `db.${query.table}.aggregate(${JSON.stringify(pipeline, null, 2)})`;
  }

  private generateInsert(query: SQLQuery): string {
    const doc = {};
    if (query.values) {
      query.values.forEach((val, idx) => {
        const field = `field_${idx}`;
        Object.assign(doc, { [field]: val });
      });
    }
    return `db.${query.table}.insertOne(${JSON.stringify(doc, null, 2)})`;
  }

  private generateUpdate(query: SQLQuery): string {
    const filter = query.where ? this.buildMatch(query.where) : {};
    return `db.${query.table}.updateMany(${JSON.stringify(filter, null, 2)}, { $set: { /* values */ } })`;
  }

  private generateDelete(query: SQLQuery): string {
    const filter = query.where ? this.buildMatch(query.where) : {};
    return `db.${query.table}.deleteMany(${JSON.stringify(filter, null, 2)})`;
  }

  private buildMatch(where: WhereClause): Record<string, any> {
    const match: Record<string, any> = {};

    const buildCondition = (cond: WhereClause) => {
      const field = `$${cond.field}`;
      const operatorMap: Record<string, string> = {
        '=': '$eq',
        '>': '$gt',
        '<': '$lt',
        '>=': '$gte',
        '<=': '$lte',
        '!=': '$ne',
        '<>': '$ne',
      };

      const op = operatorMap[cond.operator] || '$eq';
      return { [cond.field]: { [op]: cond.value } };
    };

    return buildCondition(where);
  }
}

// Main Transcompiler
export class SQLToNoSQLTranscompiler {
  private analyzer = new SemanticAnalyzer();

  compile(sql: string): CompilationResult {
    const result: CompilationResult = {
      success: false,
      mongodb: '',
      errors: [],
      warnings: [],
      phases: { lexical: [], syntax: [], semantic: [] },
    };

    // Phase 1: Lexical Analysis
    const lexer = new Lexer(sql);
    const { tokens, errors: lexicalErrors } = lexer.tokenize();
    result.phases.lexical = lexicalErrors;
    result.errors.push(...lexicalErrors);

    if (lexicalErrors.length > 0) {
      return result;
    }

    // Phase 2: Syntax Analysis
    const parser = new Parser(tokens);
    const { query, errors: syntaxErrors } = parser.parse();
    result.phases.syntax = syntaxErrors;
    result.errors.push(...syntaxErrors);

    if (!query || syntaxErrors.length > 0) {
      return result;
    }

    // Phase 3: Semantic Analysis
    const semanticErrors = this.analyzer.analyze(query);
    result.phases.semantic = semanticErrors;
    result.errors.push(...semanticErrors);

    if (semanticErrors.length > 0) {
      return result;
    }

    // Phase 4: ICG Generation
    const generator = new ICGGenerator();
    result.mongodb = generator.generate(query);
    result.success = true;

    return result;
  }

  getSchema(): Record<string, string[]> {
    return this.analyzer.getSchema();
  }
}
