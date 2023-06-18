/*
 * Copyright (c) 2023 ColleagueRiley ColleagueRiley@gmail.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following r estrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *
 */

#define STB_C_LEXER_IMPLEMENTATION
#define SI_IMPLEMENTATION

#include "sili.h"

#include "stb_c_lexer.h"

#include <assert.h>

/* shortened function names */
#define strAppend si_string_append
#define strAppendL si_string_append_len

/* text macros */
#define RESET "\033[0m"
#define RED "\033[31m" /* Red */
#define BOLD "\033[1m"
#define textRed(text) RED, text, RESET
#define textBold(text) BOLD, text, RESET

char *double_to_string(double num) {
  static char string[23];
  sprintf(string, "%f", num);

  return string;
}

unsigned int indent = 0; /* level of indentation */

/*
1: found struct (keyword)
2: inside struct definition
*/
unsigned int structMode;
siString structName;       /* name of found struct */
bool typedefCheck = false; /* if the found struct is in a typedef or not */

siArray(siString) structFuncs = NULL; /* functions defined inside the struct */

typedef struct object {
  char *varName;       /* name of the object    */
  char *varType;       /* the var type (the class of the object)  */
  unsigned int indent; /* the indent level of the define (to get the scope) */
} object;

siArray(siString) classes; /* array of all the classes */
siArray(object) objs;      /* array of all the objects, stored as `object` structs */

siString namespace;           /* string of the current namespace (if we're in an namespace) */
siArray(siString) namespaces; /* list of all the saved namespaces */
bool NSfunc = false; /* if we just handled a function in a namespace (so we don't repeat the `;` token ) */

#define CPLUS_ERROR(file, error, lex) {\
  stb_lex_location loc;\
  stb_c_lexer_get_location(&lex, lex.where_firstchar, &loc);\
  \
  printf("%s:%i:%i: %s%s%s %s\n", file, loc.line_number, loc.line_offset, textRed("error"), error);\
  exit(1);\
}

siString *handle_token(stb_lexer *lexer, siString *c_code, char* file) {
  /* double tabs are used for switches here */
  
  int i; /* used in for loops */
  
  switch (lexer->token) {
      case CLEX_id: {
        if (si_strings_are_equal(lexer->string, "namespace")) {
          stb_c_lexer_get_token(lexer); /* get the next token */

          si_string_set(&namespace, lexer->string); /* set the current namespace to the next token (which should be the namespace's name) */

          si_array_append(&namespaces, lexer->string); /* append the namespace to the namespace list for checking later */

          /* check if the next token is a {, if it's not there's an error */
          stb_lexer lex = *lexer;
          stb_c_lexer_get_token(&lex);
          
          if (lex.token != '{')
            CPLUS_ERROR(file, "  expected a '{'", lex);

          break;
        }

        else if (si_strings_are_equal(lexer->string, "typedef")) { /* if the current token is typedef */
          stb_lexer lex = *lexer;
          stb_c_lexer_get_token(&lex);


          if (si_strings_are_equal(lex.string, "struct")) /* if the next token is a struct */
            typedefCheck = true; /* there was a typedef [before a struct token] */

          /* 
            @todo: way to handle typedef for objects / namespaces
          */
         
          lexer->string = "typedef";
        }

        else if (si_strings_are_equal(lexer->string, "struct")) { /* if the current token is struct */
          structMode++; /* activate structmode (this is so we can start collecting data for structs so we can make them into objects if we need to and to handle typedef) */
      
          if (!typedefCheck) /* if there wasn't a typedef before this struct define */
            strAppendL(c_code, "typedef ", 8); /* add our own typedef */

          strAppendL(c_code, "struct ", 7); /* add the `struct` token */

          stb_c_lexer_get_token(lexer); /* get the next token which should be the struct's name */
          
          structName = si_string_make(lexer->string); /* add the struct's name to the code */

          /* 
            now it should look like this

            typedef struct `struct name`
          */

          if (si_string_len(namespace)) { /* if the struct is defined in a namespace */
            /* add `namespace`_ to the structName */

            si_string_insert(&structName, namespace, -1); 
            si_string_insert(&structName, "_", si_string_len(namespace) - 1);
          }

          strAppend(c_code, structName); /* append the struct token to the code */
          si_string_push(c_code, ' '); /* add a space for the next token */

          /* check if the next token is a { OR ;, if it's not there's an error */
          stb_lexer lex = *lexer;
          stb_c_lexer_get_token(&lex);
          
          if (lex.token != '{' && lex.token != ';')
            CPLUS_ERROR(file, "  expected a '{' or ';'", lex);

          break;
        }

        siString NS = si_string_make(""); /* namespace opperator before the token, if there is any */

        if (si_string_back(*c_code) == '_') { /* if the last char is a _, there's probably a namespace opperator */
          /* the index of the char after the last space in the c_code, this means the space before the _, this is so we can find "`namespace`_ and then turn it into `namespace` via pop */
          i = si_string_rfind(*c_code, " ") + 1;

          si_string_append(&NS, *c_code + i); /* add the namespace to NS */
          si_string_pop(&NS); /* remove the extra _ */
        }

        for (i = 0; i < si_array_len(classes); i++) { /* iterate through the saved classes */
          siString className = si_string_make(NS); /* make a siString from NS so we don't write directly on NS */ 
          
          if (si_string_len(className)) /* if there was a namespace */
            si_string_push(&className, '_'); /* add "_`class name`" to fully class name */

          si_string_append(&className, lexer->string); 

          if (si_strings_are_equal(classes[i], className)) { /* if the current token is a class */
            siString ogStr = si_string_make(lexer->string); /* grab original string (structure name w/o namespace) */

            object o = {"", classes[i], indent}; /* make a new object */

            /* get the object's name */
            stb_lexer lex = *(const stb_lexer *)lexer;
            stb_c_lexer_get_token(&lex);

            while (lex.token == '*') /* if it's a pointer, check the next token until you get the name */
              stb_c_lexer_get_token(&lex);

            o.varName = strdup(lex.string); 

            /* this segs fault when you create moree than 3 objs */
            assert(objs != NULL);

            si_array_append(&objs, NULL);

            objs[si_array_len(objs) - 1] = o; /* push the object into objs */

            si_string_append(c_code, ogStr);

            si_string_free(ogStr); /* free original string */
            lexer->string = ""; /* set the next token to nothing so it isn't appended */
            break;
          }
          
          si_string_free(className);
        }

        si_string_free(NS); /* free NS */

        strAppend(c_code, lexer->string); /* append the token's string to the code */
        break;
      }

      /* handle simple 2-3 character tokens */
      case CLEX_eq: strAppendL(c_code, "==", 2); break;
      case CLEX_noteq: strAppendL(c_code, "!=", 2); break;
      case CLEX_lesseq: strAppendL(c_code, "<=", 2); break;
      case CLEX_greatereq: strAppendL(c_code, ">=", 2); break;
      case CLEX_andand: strAppendL(c_code, "&&", 2); break;
      case CLEX_oror: strAppendL(c_code, "||", 2); break;
      case CLEX_shl: strAppendL(c_code, "<<", 2);  break;
      case CLEX_shr: strAppendL(c_code, ">>", 2); break;
      case CLEX_plusplus: strAppendL(c_code, "++", 2); break;
      case CLEX_minusminus: strAppendL(c_code, "--", 2); break;
      case CLEX_andeq: strAppendL(c_code, "&=", 2); break;
      case CLEX_oreq: strAppendL(c_code, "|=", 2); break;
      case CLEX_xoreq: strAppendL(c_code, "^=", 2); break;
      case CLEX_pluseq: strAppendL(c_code, "+=", 2); break;
      case CLEX_minuseq: strAppendL(c_code, "-=", 2); break;
      case CLEX_muleq: strAppendL(c_code, "*=", 2); break;
      case CLEX_diveq: strAppendL(c_code, "/=", 2); break;
      case CLEX_modeq: strAppendL(c_code, "%%=", 2);  break;
      case CLEX_shleq: strAppendL(c_code, "<<=", 3); break;
      case CLEX_shreq: strAppendL(c_code, ">>=", 3); break;
      case CLEX_eqarrow: strAppendL(c_code, "=>", 2); break;

      /* handle string/char/ */
      case CLEX_dqstring: /* "`lexer->string`" */
        strAppendL(c_code, "\"", 1);
        strAppendL(c_code, lexer->string, lexer->string_len);
        strAppendL(c_code, "\"", 1);
        break;
      case CLEX_sqstring: /* '"`lexer->string`"' */
        strAppendL(c_code, "\"'", 2);
        strAppendL(c_code, lexer->string, lexer->string_len);
        strAppendL(c_code, "\"'", 2);
      case CLEX_charlit: /* '`lexer->string' */
        strAppendL(c_code, "'", 1);
        strAppendL(c_code, lexer->string, lexer->string_len);
        strAppendL(c_code, "'", 1);

      /* handle int/float tokens */
      case CLEX_intlit:
      case CLEX_floatlit:
        strAppend(c_code,
                  (lexer->token == CLEX_floatlit) ? 
                    double_to_string(lexer->real_number) : 
                    si_i64_to_cstr(lexer->int_number)
                );
        break;
      
      case CLEX_arrow: {
        /* check if there is a function */
        stb_lexer lex;
        for (lex = *lexer; lex.token != ';' && lex.token != '('; stb_c_lexer_get_token(&lex));
        
        if (lex.token == '(')
          goto CPLUS_DOT; /* if it's a function, let the '.' token handler handle it as to not repeat a lot of code */

        strAppendL(c_code, "->", 2);
        break;
      }

      /*
       * chars * 
      */

      case '{':
        if (structMode) /* if we're creating an class, we're going further into the class, so add 1 to structMode */
          structMode++;

        indent++; /* 
                    if there's an {, the indenting/scope level is going up 
                    this means it will also effect initializer lists but it won't cause any issues [I think]
                  */

        if (si_string_len(namespace) && indent == 1)  /* if we're writing into a namespace, we can leave here since namespace doesn't actually want the { token writen */
          break;

        si_string_push(c_code, lexer->token); /* write the '{' token */
        strAppendL(c_code, "\n", 1); /* add a newline (for clean code) */
        break;

      case '}':
        if (structMode) /* if we're creating an class, we're leaving a scope of the class, so remove 1 to structMode */
          structMode--;

        indent--; /* 
                    if there's an }, the indenting/scope level is going down 
                    this means it will also effect initializer lists but it won't cause any issues [I think] [2]
                  */

        if (si_string_len(namespace) && !indent) {
          strAppendL(c_code, "\n", 1);

          si_string_set(&namespace, "");
          break;
        }

        si_string_erase(c_code, si_string_len(*c_code) - 2, 2); /* delete the extra spaces so the '}' matches the new indent level */
        si_string_push(c_code, lexer->token); /* push the } token */

        if (structMode == 1 && typedefCheck) /* if we're writing a class and there's a typedef before it, add a space for the structure's name */
          strAppendL(c_code, " ", 1);

        strAppendL(c_code, "\n\n", 2); /* add two newlines for cleaner code */
        
        NSfunc = false; /* incase if NSfunc is still on because there was no `;` token, turn it off now */
        break;

      case '.': {
        CPLUS_DOT: /* this is for handling :: [for namespaces] and -> [for pointers] since it's handled the same way as '.' for namespaces */

        if (si_string_back(*c_code) == ' ') /* if there is an extra space before the ' ' [cplus should've created this but it might be good to check anyway] */
          si_string_pop(c_code); /* delete it! */

        /* the index of where the last space is (+ 1), so we can get the text before the . token, this text should be the object's name*/
        i = si_string_rfind(*c_code, " ") + 1;
        siString objectName = si_string_make(*c_code + i);

        int xx = -1; /* for checking if the object name was found or not (and thereby if it is an object or not) */

        for (i = 0; i < si_array_len(objs); i++) { /* go through the saved objs */
          if (si_strings_are_equal(objs[i].varName, objectName) && /* if index's object name matches our object's name AND */ 
              objs[i].indent <= indent && /* make sure it's not in a higher scope AND */
              (xx != -1 || objs[i].indent > objs[xx].indent)) /* if we already found the object index that matches, see if this one is closer in scope than the last found one */
                  xx = i; /* we found a matching one in a readable scope! */
        }

        if (xx != -1) {
          char* funcName; 

          /*  this should move the `lex` lexer token to where the function name is, or if there is a function call in the firstplace */
          stb_lexer lex;
          for (lex = *lexer; lex.token != ';' && lex.token != '('; stb_c_lexer_get_token(&lex))
            funcName = lex.string; /* grab the function name */

          if (lex.token == '(') { /* we've an object and we're running a function */
            siString func = si_string_make("cplus_"); /* all class functions start with cplus_ */

            si_string_append(&func, objs[xx].varType); /* then it has the class name after it */
            si_string_push(&func, '_'); /* then it has a seperator */
            si_string_append(&func, funcName); /* finally the actual function name */

            /* to create "cplus_`varType`_`funcName` */

            si_string_push(&func, '('); /* add the ( to open the function*/
            
            if (lexer->token != CLEX_arrow) /* if it's already a pointer, don't add & */
              si_string_push(&func, '&'); /*and a & because the first argument for all classfunctions is a pointer to the object you want to edit (eg. class* this) */
            
            si_string_append(&func, objectName); /* then of course add the actual object to get the pointer of */

            /* cplus_`varType`_`funcName`(&`objectName` */

            /* get the next token */
            stb_c_lexer_get_token(&lex); 
            if (lex.token != ')') /* if the next token isn't ) then there are some more arguments to the function */
              si_string_append_len(&func, ", ", 2); /* add a comma and space for syntax  cplus_`varType`_`funcName`(&`objectName`,  */

            for (i = 0; i < 3; i++) 
              stb_c_lexer_get_token(lexer); /* get the actual lexer to the same token as `lex` */

            /* copy the rest of the function onto the func string */
            for (lexer; lexer->token != ';'; stb_c_lexer_get_token(lexer)) 
              handle_token(lexer, &func, file);
              
            /* 
              because of how the lexer works, it adds an extra space to the end, 
              we don't need an extra space so we'll just replace it with a semicolon since we need that 
            */
            func[si_string_len(func) - 1] = ';';
            
            /* erase the object name from the c code */
            si_string_erase(c_code, si_string_len(*c_code) - si_string_len(objectName), si_string_len(objectName));
            si_string_append(c_code, func); /* add the completed function call */
            si_string_push(c_code, '\n'); /* add a newline for a cleaner output */

            si_string_free(func); /* free the func siString */

            si_string_free(objectName); /* free the objectName siString since we don't need it anymore and break since we don't want the '.' token */
            break; 
          }
        }

        else { /* if an object was not found */
          /* see if we can find an namespace instead */
          for (i = 0; i < si_array_len(namespaces); i++) { /* iterating through the namespaces */
            if (si_strings_are_equal(namespaces[i], objectName)) { /* we found a matching namespace */
              xx = 1; /* say we found one */ 
              break;
            }
          }

          if (xx != -1) {  
            /* 
              if the namespace is found, replace . with _ because a namespace is just "`namespace`_" in our c code, 
              so "`namespace`." can easily be converted just by changing the . to _ 
            */
            strAppendL(c_code, "_", 1);
            
            si_string_free(objectName); /* free the objectName siString since we don't need it anymore and break since we don't want the '.' token */
            break; 
          }
        }

        si_string_free(objectName); /* free the objectName siString since we don't need it anymore */

        si_string_push(c_code, lexer->token); /* push the '.' token */
        break;
      }

      case '(': {
        if (structMode > 1) { /* if we're reading from an class */
          siString func = si_string_make("cplus_"); /* created function  (class functions always start with cplus_) */

          si_string_append(&func, structName); /* add structure name  */
          si_string_push(&func, '_'); /* add seperator between the class name and the function name  */

          si_string_pop(c_code); /* remove the extra space (so we can get the function info) */

          /* the index of where the last tab is (+ 1), so we can get the text before the . token, this text should be the function's name + data type*/
          i = si_string_rfind(*c_code, "  ") + 1;
          
          siString piece = si_string_make(*c_code + i); /* make a siString copy of the piece used for splliting the function data  */
          siArray(siString) split = si_string_split(piece, " "); /* split into function return type / function name */

          /* remove leftover function data from c-code [+ 2 for the indent / newline] */
          si_string_erase(c_code, si_string_len(*c_code) - si_string_len(piece) - 2, si_string_len(piece) + 2);

          si_string_free(piece); /* free the piece since we don't need it anymore */

          si_string_append(&func, split[1]); /* append the functions name */

          si_string_insert(&func, " ", -1); /* add a space in front of the function to seperate the return type from the function */
          si_string_insert(&func, split[0], -1); /* add the return type */

          si_array_free(split); /* free split since we don't need it anymore */
          /* it should look like this at this point 
            cplus_`structName`_`funcName`
          */

          si_string_push(&func, '('); /* add function opening  */

          /* add `structType`* this, because all class functions have a pointer to the object you want to edit called 'this' */
          si_string_append(&func, structName);
          si_string_append_len(&func, "* this", 6);


          /* look ahead */
          stb_lexer lex = *lexer;
          stb_c_lexer_get_token(&lex);

          if (lex.token != ')') /* if the next token is not a ')', then there are more args and there should be a , for "this" */
            si_string_append_len(&func, ", ", 2);


          bool inBrackets = false;

          while(1){
              stb_c_lexer_get_token(lexer); /* get the next token */

              handle_token(lexer, &func, file); /* handle the next token directly into the func string */

              if (!inBrackets && lexer->token == '{') /* if there is a '{' token we can't end at ';' anymore */
                inBrackets = true;

              if ((lexer->token == ';' && !inBrackets) || /* if the next line is not ;, or if it's in brackets in which case we should only wait for } */
                  lexer->token == '}') /* if } the function ended */
                    break; /* break out of the while loop (I use this method so that way the } and ; are read)*/
          }

          if (inBrackets)
            si_string_erase(&func, si_string_len(func) - 7, 2); /* remove indent for the close bracket */

          si_array_append(&structFuncs, func); /* func to the list of functions for this class (to be writen into c_plus when the class is done [see ';']) */

          break;
        }

        else if (si_string_len(namespace)) { /* if we're writing a namespace */
          si_string_pop(c_code); /* erase the extra space */
          i = si_string_rfind(*c_code, " "); /* find the last space, (this should be where the function name starts so we can add the namespace name in front of it) */

          /* add `namespace`_ to the c string */
          si_string_insert(c_code, "_", i);
          si_string_insert(c_code, namespace, i);

          NSfunc = true; /* we just wrote a function don't repeat the `;` token thing  */
        }

        si_string_push(c_code, lexer->token);
        break;
      }

      case ':': {
        /* look ahead */
        stb_lexer lex = *lexer; 
        stb_c_lexer_get_token(&lex);

        if (lex.token == ':') { /* if the complete token is '::' it could be used as an namespace or for external function defines for classes '. '*/
          int xx = -1;
          if (!indent) { /* if we're in global, check if we're using a namespace or not */\
            bool addSpaceBack = false; /* if we removed a space, remind ourselves to add it back [if needed] */

            if (si_string_back(*c_code) == ' ') { /* if the last char is a space, remove the space */
              si_string_pop(c_code); /* [ this is so when we look for the last space we find the last space behind what we think is the namespace nad not this one ] */
              addSpaceBack = true; /* remind ourselves later we removed the space */
            }

            i = si_string_rfind(*c_code, " ") + 1; /* find space before the namespace */
            siString namespaceName = si_string_make(*c_code + i);

            for (i = 0; i < si_array_len(namespaces); i++) { /* iterating through the namespaces */
              if (si_strings_are_equal(namespaces[i], namespaceName)) { /* we found a matching namespace */
                xx = 1; /* say we found one */ 
                break;
              }
            }

            if (xx == -1 && addSpaceBack)  /* if a namespace wasn't found and a space was removed */
              si_string_push(c_code, ' '); /* add the space back */

            si_string_free(namespaceName); /* free namespaceName because it's not needed anymore */
          }

          if (xx != -1 || indent) { /* if we're in a namespace, handle it like the '.' token */

            stb_c_lexer_get_token(lexer); /* actually move the current lexer ahead (and skip this* token) */

            lexer->token = '.'; /* set the current token to . since it doesn't need to be ':' anymore */

            goto CPLUS_DOT; /* goto where the '.' token is handled*/
          }

          if (!indent) { /* otherwise, if we're in global it should be handled like an externally defined class function */
            stb_c_lexer_get_token(lexer); /* go to next token */
            
            si_string_pop(c_code); /* remove the extra space */
            si_string_push(c_code, '_'); /* replace '::' with _ */

            i = si_string_rfind(*c_code, " ") + 1; /* get the class name based on the last space */
            siString className = si_string_make(*c_code + i);
            si_string_pop(&className); /* this == `className`_ because it's apart of the function, so remove the extra _ with pop */

            /* add "cplus_" before the function because all c++ class functions have cplus_ before them */
            si_string_insert(c_code, "cplus_", si_string_len(*c_code) - si_string_len(className) - 2);

            while (lexer->token != ')') { /* loop through the token until ) (or ( actually) */
              stb_c_lexer_get_token(lexer); /* get the next token */
              handle_token(lexer, c_code, file); /* handle the next token */
              
              if (lexer->token == '(') { /* if the next token is a `(` */
                si_string_append(c_code, className); 
                si_string_append(c_code, "* this"); /* add "`className`* type" to the c code string because all class functions require a pointer of the object you want to edit */

                stb_lexer lex = *lexer; /* check the next token */
                stb_c_lexer_get_token(&lex);
                
                if (lex.token != ')') /* if the next token isn't a ')' token, then it's an arg so add a ", " for syntax */
                  si_string_append(c_code, ", ");

                break; /* break out of the while loop because we don't need it anymore */
              }
            }

            si_string_free(className); /* free the className because we don't need it anymore */
          }
        }

        else
          si_string_push(c_code, lexer->token); /* otherwise just push the ':' token */
        break;
      }

      case ';': {
        /* cleaning up syntax */
        
        if (structMode == 1 && typedefCheck) /* if we're finishing up an class and there was a typedef */
          si_string_erase(c_code, si_string_len(*c_code) - si_string_len(structName) - 3, 2); /* remove the extra \n */

        else if (((*c_code)[si_string_len(*c_code) - 2] == '\n')) /* if there is \n before the ; token, remove it*/
          si_string_erase(c_code, si_string_len(*c_code) - 2, 1);
        
        si_string_pop(c_code); /* remove the extra space */

        /*  */

        /* typedefless struct support  (if we're finishing up an class and there was no typedef) */
        if (structMode == 1 && !typedefCheck) { 
          /* add a space followed by the class's name */
          si_string_push(c_code, ' ');
          strAppend(c_code, structName);

          typedefCheck = false; /* we're doing with working on the struct so typedefCheck can be toggled back off */
        }

        si_string_push(c_code, lexer->token); /* add the `;` token to the c code */
        si_string_push(c_code, '\n'); /* add a newline after the ';' */

        /* finish defining classes and handle namespaces */

        if (structMode == 1) { /* classes */
          si_string_push(c_code, '\n'); /* add another newline to the code for cleanness */
          
          si_array_append(&classes, structName); /* add the new class to the class list */

          int i, j;

          for (i = 0; i < si_array_len(structFuncs); i++) { /*iterate through all the saved functions for this class */
            si_string_append(c_code, structFuncs[i]); /* write the function to the c code */
            si_string_append(c_code, "\n"); /* add a newline for cleanness */
          }

          SI_ARRAY_HEADER(structFuncs)->len = 0; /* reset structFuncs since we're done with them */
          structMode = 0; /* reset structMode because we're in in a struct anymore */
        }

        else if (si_string_len(namespace) && indent == 1) { /* handle namespaces [write `namespace`_ to the variable inside the namespace] */
          if (NSfunc) {
            NSfunc = false;
            break;
          }
          
          for (i = si_string_len(*c_code) - 2; i >= 0; i--) { /* try to find where the variable name is */
            if ((*c_code)[i] == ' ' && (*c_code)[i - 1] == '=' && i - 2 >= 0)
              i -= 2;  /* if there is a space with a = after, skip over to the next part */

            else if ((*c_code)[i] == ' ') /* if we found a space, we found where the variable name is */
              break;
          }

          /* write `namespace`_ in front of the variable name, thus adding it to the namespace (in c) */
          si_string_insert(c_code, namespace, i); 
          si_string_insert(c_code, "_", i + si_string_len(namespace));
        }

        break;
      }

      default: 
        if (lexer->token >= 0 && lexer->token < 256) /* if it's a valid token (0 - 256) */
          si_string_push(c_code, lexer->token); /* there's no handling to do, just push the token */

        else /* no valid token was found */
          CPLUS_ERROR(file, "unknown token", (*lexer));

        break;
  }

  

  if (si_string_back(*c_code) == '\n') /* if we're on a newline */
    for (i = 0; i < indent * 2; i++) /* add indenting based on indent level */
      strAppendL(c_code, " ", 1); 

  /* if it's not a newline and it's not one of these tokens, add a space */
  else if (si_string_back(*c_code) != '.' && si_string_back(*c_code) != '_') 
    strAppendL(c_code, " ", 1); 
}

int main(int argc, char **argv) {
  if (argc == 1) { /* if there are no args, there are no files given */
  CPLUS_NO_FILE: /* for calling back if there are no files given */

    printf("%s%s%s: %s%s%s: no input files\ncompilation terminated.\n",
           textBold(argv[0]),
           textRed("fatal error"));

    /*argc = 3;
    argv[1] = "test.cp";
    argv[2] = "-no-compile";*/
    return 0;
  }

  bool no_compile = false; /* compile the c_code? */
  char *outputName = "output.c"; /* output name for the c file */
  char *compiler = "gcc"; /* c compiler to use */

  
  siString c_args = si_string_make(""); /* args for the c compiler */

  siArray(char *) files = si_array_make_reserve(sizeof(char *), 0); /* cplus files to compile */

  int i;
  for (i = 1; i < argc; i++) { /* loop through the args */
    if (argv[i][0] == '-') { /* if the first char is a - check for args */
      if (si_strings_are_equal("-cc", argv[i]) && no_compile) /* arg to set the compiler */
        compiler = argv[i++];

      else if (si_strings_are_equal("-no-compile", argv[i])) /* rag to turn on no compile */
        no_compile = true;

      else if (si_strings_are_equal("-o", argv[i]) && no_compile) /* arg to set the output file [only needed if no_compile is on, otherwise it's a c-arg] */
        outputName = argv[i++];

      else { /* else add it to c args*/
        si_string_push(&c_args, ' ');
        si_string_append(&c_args, argv[i]);
      }
    }

    else if (!si_string_len(c_args)) { /* if c args aren't being collected */
      si_array_append(&files, argv[i]); /* add the arg to files */

      if (!si_path_exists(argv[i])) { /* check if the file exists */
        printf("No such file or directory \"%s\"\n", argv[i]); /* send an error if it doesn't exist */

        exit(1);
      }
    }

    else if (si_string_len(c_args)) { /* else, if it's collecting c args, add it to the c args [if it wasn't already collected] */
      si_string_push(&c_args, ' ');
      si_string_append(&c_args, argv[i]);
    }
  }

  if (!si_array_len(files)) /* if there are no files collected, go to no files error */
    goto CPLUS_NO_FILE;

  siFile f = si_file_open(files[0]); /* open and read the first file into `text` */
  siString text = si_file_read(f);
  si_file_close(f); /* close the file because it's not reuiqred anymore */

  stb_lexer lex;
  siString c_code = si_string_make("#define __cplus__\n\n"); /* add cplus macro */

  char *stringStore = malloc(0x10000); /* for string storage */
  stb_c_lexer_init(&lex, text, text + f.size, stringStore, 0x10000); /* init the c lexer for stb */

  /* global arrays and the namespace string */
  classes = si_array_make_reserve(sizeof(siString), 0);
  objs = si_array_make_reserve(sizeof(object), 0);

  structFuncs = si_array_make_reserve(sizeof(siString), 0);
  namespaces = si_array_make_reserve(sizeof(siString), 0);

  namespace = si_string_make("");

  while (stb_c_lexer_get_token(&lex)) { /* iterate through and handle the tokens */
    if (lex.token == CLEX_parse_error) {
      printf("\n<<<PARSE ERROR>>>\n"); 
      break;
    }

    handle_token(&lex, &c_code, files[0]);
  }

  f = si_file_create(outputName); /* create and write to the output  c file */
  si_file_write(&f, c_code);
  si_file_close(f); /* close the file because we don't need it anymore */

  /* free classes, c_code and stringStore because we don't need it anymore */
  si_array_free(classes);

  si_string_free(c_code);
  free(stringStore);

  if (!no_compile) { /* if compiling the c output is allowed*/
    /* generate the compile command */
    siString cmd = si_string_make(compiler);
    si_string_push(&cmd, ' ');
    si_string_append(&cmd, outputName);

    si_string_append(&cmd, c_args);

    system(cmd); /* compile the c code */
    si_path_remove("output.c"); /* remove the c output file  */
  
    si_string_free(cmd); /* free the command strign because we're not using it anymore */
  }

  /* free the rest of the allocated data */
  si_string_free(c_args);
  si_array_free(files);
 
  if (si_array_len(namespaces))
    si_array_free(namespaces);
}