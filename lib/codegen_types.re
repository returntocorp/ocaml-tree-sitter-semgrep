module B = Ast_grammar_normalized;

let counter = ref(0);
let gen_intermediate_constructor = () => {
  incr(counter);
  "Intermediate_type" ++ string_of_int(counter^);
}

let wrap_ident = (ident:string) : string => {
   if (ident == "true" || ident == "false") {
      ident ++ "_bool"
   } else {
      ident
   }
}
let name_to_constructor = (name: string): string => {
   if (name.[0] == '_') {
      /*
         For intermediate types like _a_type
         convert to constructor name like A_type_
       */
      String.capitalize_ascii(String.sub(name, 1, String.length(name) - 1) ++ "_")
   } else {
      String.capitalize_ascii(name)
   }
}


let codegen_atom = (atom: B.atom): string => {
   switch(atom) {
   | B.TOKEN => "string" /* tokens are string */
   | B.SYMBOL(name) => wrap_ident(name)
   }
}

let codegen_simple =  (simple: B.simple): string => {
   switch(simple) {
   | B.ATOM(atom) => codegen_atom(atom)
   | B.SEQ(atoms) => {
      /* codegen: (A,B,C,...) */
      let atom_strs = List.map(codegen_atom, atoms);
      "(" ++ String.concat(", ", atom_strs) ++ ")"
      }
   }
}

let codegen_rule_body = (rule_body: B.rule_body): string => {
   switch (rule_body) {
   | B.SIMPLE(simple) => {
      codegen_simple(simple);
      }
   | B.CHOICE(simples) => {
      /* codegen: A(...) | B(...) */
      let rhs = List.map(codegen_simple, simples);
      let im_types = List.map(
         (im_type: string) => {
            gen_intermediate_constructor() ++ "(" ++ wrap_ident(im_type) ++ ")"
         },
         rhs
      );
      "\n | " ++ String.concat("\n | ", im_types);
   }
   | B.REPEAT(simple) => {
      /* codegen: list(x) */
      let rhs = codegen_simple(simple);
      "list(" ++ rhs ++ ")"
   }
   | B.OPTION(simple) => {
      /* codegen: option(...) */
      let rhs = codegen_simple(simple);
      "option(" ++ rhs ++ ")"
   }
   }
}

let sort_by_type = ((a_i, a_body): B.rule, (b_i, b_body): B.rule) => {
   if (a_i == "program") {
      /* make sure program is at last */
      1;
   } else if (b_i == "program") {
      -1;
   } else {
      let a_type = B.show_rule_body(a_body);
      let b_type = B.show_rule_body(b_body);
      /* make sure leaves are at top */
      switch(a_body) {
      | B.SIMPLE(B.ATOM(B.TOKEN)) => -1
      | _ => {
         switch(b_body) {
         | B.SIMPLE(B.ATOM(B.TOKEN)) => 1
         | _ => String.compare(a_type, b_type)
         }
      }
      }
   }
}
let codegen_rule_bodies = (rules: list(B.rule)): list((B.ident, string)) => {
   let rules_sorted_by_type = List.sort(sort_by_type, rules);
   List.map(((name, body): B.rule) => {
      (name, wrap_ident(name) ++ " = " ++ codegen_rule_body(body))
   }, rules_sorted_by_type)
}

let codegen_rule_constructor = (rule_names: list(string)): string => {
   "type cst_node = \n" ++
      String.concat("\n",
         List.map(
            (name: string) => {
               let name_wrap = wrap_ident(name);
               " | " ++ name_to_constructor(name_wrap) ++ "(" ++ name_wrap ++ ")"
            },
            rule_names
         )
      ) ++ "\n"
}

let codegen = ((program_name, rules): B.grammar): (string) => {
  counter := 0;
  let code_header = "/* Auto-generated by codegen_type */\n";
  let rule_strs = codegen_rule_bodies(rules);
  let rule_names = List.map(fst, rule_strs);
  let rule_types = List.map(snd, rule_strs);
  let type_defs = "type " ++ String.concat("\nand ", rule_types) ++ ";\n";
  let type_constructors = codegen_rule_constructor(rule_names);
  let final = "type " ++ program_name ++ "_cst" ++ " = list(cst_node)";
  code_header ++ type_defs ++ type_constructors ++ final;
}
