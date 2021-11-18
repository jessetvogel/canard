// let TEST_STR = "modus_tollens (λ (p : algebraic_geometry.scheme.zariski_local algebraic_geometry.scheme.connected) := algebraic_geometry.du_zar_lc (algebraic_geometry.cn_of_int (algebraic_geometry.spec_int commutative_algebra.ZZ_is_dm)) (algebraic_geometry.cn_of_int (algebraic_geometry.spec_int commutative_algebra.ZZ_is_dm)) p) (algebraic_geometry.du_not_cn (algebraic_geometry.spec_not_empty commutative_algebra.ZZ_not_trivial) (algebraic_geometry.spec_not_empty commutative_algebra.ZZ_not_trivial))"
// requestGET('/doc/definitions.json').then(response => window.DEFINITIONS = JSON.parse(response));

// const IDENTIFIER = 'identifier';
// const WORD = 'word';

// class Lexer {
//     constructor(input) {
//         this.input = input;
//     }

//     token() {
//         let token = this.tokenHelper();
//         if (token != null)
//             this.input = this.input.slice(token.length);
//         return token;
//     }

//     tokenHelper() {
//         if (this.input.length == 0) return null;

//         let match;

//         match = this.input.match(/^(\w+(?:\.\w+)*)/);
//         if (match != null) return match[1];

//         match = this.input.match(/^(:=)/);
//         if (match != null) return match[1];

//         match = this.input.match(/^([\(\)\{\}λ:])/);
//         if (match != null) return match[1];

//         match = this.input.match(/^(\s+)/);
//         if (match != null) {
//             this.input = this.input.slice(match[1].length); // remove whitespace and repeat
//             return this.tokenHelper();
//         }

//         console.warn('Unknown token?', this.input);
//         return null;
//     }
// }

// class Parser {

//     constructor(input) {
//         this.lexer = new Lexer(input);
//         this.token = null;
//     }

//     found(type) {
//         if (this.token == null)
//             this.nextToken();

//         if (this.token == null)
//             return false;

//         if (type == null)
//             return true;

//         if (this.token == type) return true;

//         if (type == IDENTIFIER && this.token.match(/^\w/)) return true;
//         if (type == WORD && this.token.match(/^\w+$/)) return true;

//         return false;
//     }

//     nextToken() {
//         this.token = this.lexer.token();
//     }

//     consume(type = null) {
//         if (this.found(type)) {
//             let token = this.token;
//             this.token = null;
//             return token;
//         } else {
//             console.warn(`expected ${type} but found ${this.token}`);
//             return null;
//         }
//     }

//     parseExpression() {
//         // Case λ ARGUMENTS := EXPRESSION
//         if (this.found('λ')) {
//             this.consume();
//             const params = [];
//             let param;
//             while ((param = this.parseParameter()) != null)
//                 params.push(param);
//             this.consume(':=');
//             const value = this.parseExpression();
//             return new Lambda(params, value);
//         }

//         // Case TERM+
//         const terms = [];
//         let term;
//         while ((term = this.parseTerm()) != null)
//             terms.push(term);

//         if (terms.length == 0)
//             console.warn('expected expression');

//         if (terms.length == 1) return terms[0];

//         return new Specialization(terms[0], terms.splice(1));
//     }
    
//     parseFunction() {
//         const identifier = this.consume(IDENTIFIER);
//         const params = [];
//         let param;
//         while((param = this.parseParameter()) != null)
//             params.push(param);
//         this.consume(':');
//         const type = this.parseExpression();
//         return new Function(identifier, params, type);
//     }

//     parseTerm() {
//         // TERM = IDENTIFIER | ( EXPRESSION )
//         if (this.found(IDENTIFIER))
//             return this.consume();

//         if (this.found('(')) {
//             this.consume();
//             let expr = this.parseExpression();
//             this.consume(')')
//             return expr;
//         }

//         return null;
//     }

//     parseParameter() {
//         if (!this.found('(') && !this.found('{')) return null;
//         const explicit = this.found('(');

//         // ( WORD : EXPRESSION ) or { WORD : EXPRESSION }
//         this.consume(explicit ? '(' : '{');
//         const name = this.consume(WORD);
//         this.consume(':');
//         const type = this.parseExpression();
//         this.consume(explicit ? ')' : '}');
//         return new Parameter(name, type, explicit);
//     }
// }

// class Parameter {
//     constructor(name, type, explicit) {
//         this.name = name;
//         this.type = type;
//         this.explicit = explicit;
//     }
// }

// class Lambda {
//     constructor(params, value) {
//         this.params = params;
//         this.value = value;
//     }
// }

// class Specialization {
//     constructor(base, args) {
//         this.base = base;
//         this.args = args;
//     }
// }

// class Function {
//     constructor(name, args, type) {
//         this.name = name;
//         this.args = args;
//         this.type = type;
//     }
// }

// /* Textify functions */
// function isTheorem(identifier) {
//     return ['modus_tollens'].includes(identifier);
// }

// function isAdjective(identifier) {
//     if(['algebraic_geometry.scheme.zariski_local'].includes(identifier))
//         return true;
//     for(let t in properties) {
//         if(identifier in properties[t])
//             return true;
//     }
//     return false;
// }

// function isFunctor(identifier) {
//     return ['Spec'].includes(identifier);
// }

// function textify(expr) {
//     if(expr instanceof Specialization) {
//         if(isAdjective(expr.base))
//             return `${textify(expr.args[0])} is ${textify(expr.base)}`;
        
//         if(isFunctor(expr.base))
//         // else {
//             return `${textify(expr.base)}(${expr.args.map(x => textify(x)).join(', ')})`;
        
//         // if(isTheorem(expr.base)) {
//         else{
//             let args = expr.args.map(x => textify(x));
//             let argsText;
//             if(args.length == 0)
//                 argsText = '';
//             else
//                 argsText = ` with <ul><li>${expr.args.map(x => textify(x)).join('</li><li>')}</li></ul>`;
//             return `apply <span class="tt">${textify(expr.base)}</span>${argsText}`;
//         }
//     }

//     if(expr instanceof Lambda) {
//         const conclusion = textify(expr.value);

//         let assumptions = expr.params.map(p => textify(p));
//         if(assumptions.length == 0)
//             return conclusion;

//         if(assumptions.length == 1)
//             return `whenever ${assumptions[0]}, then <ul><li>${conclusion}</li></ul>`
        
//         else
//             return `whenever <ul>${assumptions.map(a => `<li>${a}</li>`)}</ul>, then <ul><li>${conclusion}</li></ul>`
//     }


//     if(expr instanceof Parameter) {
//         return `(<span class="tt">${expr.name}</span>) ${textify(expr.type)}`;
//     }

//     if(typeof expr == 'string')
//         return `<span class="tt">${typeset(expr)}</span>`;

//     console.warn('Unknown class');
// }

// function TEXifyObject(expr) {
//     if(expr instanceof Specialization) {
//         if(expr.base == 'algebraic_geometry.disjoint_union')
//             return `${TEXifyObject(expr.args[0])} \\sqcup ${TEXifyObject(expr.args[1])}`;
        
//         // Default
//         return `${TEXifyObject(expr.base)}(${expr.args.map(a => TEXifyObject(a)).join(', ')})`;
//     }

//     // Special cases
//     if(expr == 'Spec') return '\\Spec';
//     if(expr == 'NN') return '\\NN';
//     if(expr == 'ZZ') return '\\ZZ';
//     if(expr == 'QQ') return '\\QQ';
//     if(expr == 'RR') return '\\RR';
//     if(expr == 'CC') return '\\CC';
    
//     return `\\texttt{${expr}}`;
// }
