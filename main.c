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
#define RESET   "\033[0m"
#define RED     "\033[31m"      /* Red */
#define BOLD "\033[1m"

char* int_to_string(int num) {
  /* get how many places are in the number */
  int i, count = 10;
  for (i = 1; !(abs(num) < count); i++, count *= 10);

  char* intStr = malloc(sizeof(char) * i);
  sprintf(intStr, "%i", num);

  return intStr;
}

char* double_to_string(double num) {
  /* get how many places are in the number */
  int i, i2, count = 10;
  
  for (i = 1; !(abs(num) < count); i++, count *= 10);

  /* get how many decimal places there are */
  char* buff = malloc(sizeof(char) * 20);
  sprintf(buff, "%f", num - (int)num);

  i2 = strlen(buff) - 1;

  char* intStr = malloc(sizeof(char) * (i + i2));
  sprintf(intStr, "%f", num);

  return intStr;
}


unsigned int indent = 0; /* level of indentation */

/* 
1: found struct (keyword)
2: inside struct definition
*/
unsigned int structMode; 
siString structName; /* name of found struct */
bool typedefCheck = false; /* if the found struct is in a typedef or not */

siString namespace;

siArray(siString) structFuncs = NULL; /* functions defined inside the struct */

typedef struct object { 
    char* varName;
    char* varType; 
    unsigned int indent; 
} object;

siArray(siString) classes;
siArray(object) objs;


siArray(char*) namespaces;

siString* handle_token(stb_lexer *lexer, siString* c_code) {
  switch (lexer->token) {
    case CLEX_id        :   
              if (si_strings_are_equal(lexer->string, "namespace")) {
                stb_c_lexer_get_token(lexer);

                namespace = si_string_make(lexer->string);
                
                si_array_append(&namespaces, namespace);
                break;
              }

              if (structMode == 1) {
                structName = si_string_make(lexer->string);

                if (si_strings_are_equal(namespace, "")) {
                  si_string_insert(&structName, namespace, -1);
                  si_string_insert(&structName, "_", si_string_len(namespace) - 1);
                }
              }

              if (si_strings_are_equal(lexer->string, "struct")) {
                  structMode++;

                    if (!typedefCheck)
                      strAppendL(c_code, "typedef ", 8);
              }
      
              else if (si_strings_are_equal(lexer->string, "typedef"))
                typedefCheck = true;
              
              else if (!structMode)
                typedefCheck = false;

              int i;
              for (i = 0; i < si_array_len(classes); i++)
                if (si_strings_are_equal(classes[0], lexer->string)) {
                  si_array_append(&objs, NULL);

                  object o = {"", classes[i], indent};
                  o.varName = strdup(lexer->string);

                  objs[si_array_len(objs) - 1] = o;
                }

              strAppend(c_code, lexer->string); 
              break;
    case CLEX_eq        : strAppendL(c_code, "==", 2); break;
    case CLEX_noteq     : strAppendL(c_code, "!=", 2); break;
    case CLEX_lesseq    : strAppendL(c_code, "<=", 2); break;
    case CLEX_greatereq : strAppendL(c_code, ">=", 2); break;
    case CLEX_andand    : strAppendL(c_code, "&&", 2); break;
    case CLEX_oror      : strAppendL(c_code, "||", 2); break;
    case CLEX_shl       : strAppendL(c_code, "<<", 2); break;
    case CLEX_shr       : strAppendL(c_code, ">>", 2); break;
    case CLEX_plusplus  : strAppendL(c_code, "++", 2); break;
    case CLEX_minusminus: strAppendL(c_code, "--", 2); break;
    case CLEX_arrow     : {
        stb_lexer lex;
        char* funcName;

        for (lex = *lexer; lex.token != ';' && lex.token != '('; stb_c_lexer_get_token(&lex)) 
          funcName = lex.string;


        if (lex.token == '(') {
          siString func = si_string_make("cplus_"); 

          siString objectName = si_string_make("");

          int i;
          for (i = si_string_len(*c_code) - 2; i >= 0; i--) {
            if ((*c_code)[i] == ' ') break;
            si_string_push(&objectName, (*c_code)[i]);
          }

          si_string_reverse(&objectName);
        
          int xx = 0; 

          for (i = 0; i < si_array_len(objs); i++) {
            if (si_strings_are_equal(objs[i].varName, objectName)) {
              if (objs[i].indent <= indent && 
                  objs[i].indent > objs[xx].indent)
                xx = i;
            }
          }

          si_string_append(&func, objs[xx].varType);
          si_string_push(&func, '_');

          si_string_append(&func, funcName);

          si_string_push(&func, '(');
          si_string_append(&func, objectName);

          stb_c_lexer_get_token(&lex);
          if (lex.token != ')')
            si_string_append_len(&func, ", ", 2);

          for (i = 0; i < 3; i++)
            stb_c_lexer_get_token(lexer);

          for (lexer; lexer->token != ';'; stb_c_lexer_get_token(lexer))
            handle_token(lexer, &func);

          si_string_erase(&func, si_string_len(func) - 1, 1);
          si_string_push(&func, ';');

          si_string_erase(c_code, si_string_len(*c_code) - si_string_len(objectName), si_string_len(objectName));
          si_string_erase(c_code, si_string_len(*c_code) - 1, 1);
          si_string_append(c_code, func);
          si_string_push(c_code, '\n');

          si_string_free(func);
          si_string_free(objectName);
        } 

        else
          strAppendL(c_code, "->", 2); 
        break;
    }
    case CLEX_andeq     : strAppendL(c_code, "&=", 2); break;
    case CLEX_oreq      : strAppendL(c_code, "|=", 2); break;
    case CLEX_xoreq     : strAppendL(c_code, "^=", 2); break;
    case CLEX_pluseq    : strAppendL(c_code, "+=", 2); break;
    case CLEX_minuseq   : strAppendL(c_code, "-=", 2); break;
    case CLEX_muleq     : strAppendL(c_code, "*=", 2); break;
    case CLEX_diveq     : strAppendL(c_code, "/=", 2); break;
    case CLEX_modeq     : strAppendL(c_code, "%%=", 2); break;
    case CLEX_shleq     : strAppendL(c_code, "<<=", 3); break;
    case CLEX_shreq     : strAppendL(c_code, ">>=", 3); break;
    case CLEX_eqarrow   : strAppendL(c_code, "=>", 2); break;
    case CLEX_dqstring  : 
          strAppendL(c_code, "\"", 1); 
          strAppendL(c_code, lexer->string, lexer->string_len); 
          strAppendL(c_code, "\"", 1); 
          break;
    case CLEX_sqstring  : 
          strAppendL(c_code, "\"'", 2); 
          strAppendL(c_code, lexer->string, lexer->string_len); 
          strAppendL(c_code, "\"'", 2); 
    case CLEX_charlit   : 
          strAppendL(c_code, "'", 1); 
          strAppendL(c_code, lexer->string, lexer->string_len); 
          strAppendL(c_code, "'", 1); 
    case CLEX_intlit:
    case CLEX_floatlit: {
      char* str =  (lexer->token == CLEX_floatlit) ? 
                double_to_string(lexer->real_number) : 
                int_to_string(lexer->int_number);

      strAppend(c_code, str); 

      free(str);      
      break;
    }
    default:
        if (lexer->token >= 0 && lexer->token < 256) {
          switch(lexer->token) {
            case '{':
                if (structMode)
                  structMode++;
                
                indent++;

                if (si_string_len(namespace) && indent == 1)
                  break;

                strAppendL(c_code, &lexer->token, 1);
                strAppendL(c_code, "\n", 1);
                break;
            
            case '}':
              if (structMode)
                structMode--;
                
              indent--;

              if (si_string_len(namespace) && !indent) {
                 strAppendL(c_code, "\n", 1);
                namespace = "";
                break;
              }

              si_string_erase(c_code, si_string_len(*c_code) - 2, 2);
              strAppendL(c_code, &lexer->token, 1);
            

              if (structMode == 1 && typedefCheck)
                strAppendL(c_code, " ", 1);

              strAppendL(c_code, "\n\n", 2);
              break;

            case ';': {
                if (structMode == 1 && typedefCheck) {
                  si_string_erase(c_code, si_string_len(*c_code) - si_string_len(structName) - 3, 2);
                  si_string_erase(c_code,  si_string_len(*c_code) - 1, 1);
                }

                else if (((*c_code)[si_string_len(*c_code) - 2] == '\n'))
                  si_string_erase(c_code, si_string_len(*c_code) - 2, 2);
                else
                  si_string_erase(c_code, si_string_len(*c_code) - 1, 1);

                if (structMode == 1 && !typedefCheck) {
                  strAppendL(c_code, " ", 1);
                  strAppendL(c_code, structName, si_string_len(structName));

                  typedefCheck = false;
                }

                strAppendL(c_code, &lexer->token, 1);

                if (((*c_code)[si_string_len(*c_code) - 3] == '\n')) strAppendL(c_code, "\n", 1);
                
                strAppendL(c_code, "\n", 1);
                
                if (structMode == 1) {
                  strAppendL(c_code, "\n", 1);

                  si_array_append(&classes, structName);

                  int i, j;

                  for (i = 0; i < si_array_len(structFuncs); i++) { 
                    si_string_append(c_code, structFuncs[i]); 
                    si_string_append(c_code, "\n");
                  }

                  
                  structMode = 0;
                }

                else if (!si_strings_are_equal(namespace, "") && si_string_len(namespace) && indent == 1) {
                  int i;
                  for (i = si_string_len(*c_code) - 2; i >= 0; i--) {
                    if ((*c_code)[i] == ' ' && (*c_code)[i - 1] == '=' && i - 2 >= 0)
                      i -= 2;
                    
                    else if ((*c_code)[i] == ' ')
                      break;
                  }

                  si_string_insert(c_code, namespace, i);
                  si_string_insert(c_code, "_", i + si_string_len(namespace));
                }

                break;
            }

            case '.': { 
              CPLUS_DOT:

              if (si_string_back(*c_code) == ' ')
                si_string_erase(c_code, si_string_len(*c_code) - 1, 1);
              
              stb_lexer lex;
              char* funcName;

              for (lex = *lexer; lex.token != ';' && lex.token != '('; stb_c_lexer_get_token(&lex)) 
                funcName = lex.string;

              siString objectName = si_string_make("");

              int i;
              for (i = si_string_len(*c_code) - 1; i >= 0; i--) {
                if ((*c_code)[i] == ' ') break;
                si_string_push(&objectName, (*c_code)[i]);
              }
              
              si_string_reverse(&objectName);

              bool found = false; 
              int xx = 0;

              for (i = 0; i < si_array_len(objs); i++) {
                if (si_strings_are_equal(objs[i].varName, objectName)) 
                  if (objs[i].indent <= indent && objs[i].indent > objs[xx].indent) {
                      xx = i;
                      found = true;
                  }
              }

              if (lex.token == '(' && found) {  
                siString func = si_string_make("cplus_"); 

                si_string_append(&func, objs[xx].varType);
                si_string_push(&func, '_');

                si_string_append(&func, funcName);

                si_string_append_len(&func, "(&", 2);
                si_string_append(&func, objectName);

                stb_c_lexer_get_token(&lex);
                if (lex.token != ')')
                  si_string_append_len(&func, ", ", 2);

                for (i = 0; i < 3; i++)
                  stb_c_lexer_get_token(lexer);

                for (lexer; lexer->token != ';'; stb_c_lexer_get_token(lexer))
                  handle_token(lexer, &func);

                si_string_erase(&func, si_string_len(func) - 1, 1);
                si_string_push(&func, ';');

                si_string_erase(c_code, si_string_len(*c_code) - si_string_len(objectName), si_string_len(objectName));
                si_string_append(c_code, func);
                si_string_push(c_code, '\n');

                si_string_free(func);
              }

              else {
                
                int i;
                bool found = false;

                for (i = 0; i < si_array_len(namespaces); i++)
                  if (si_strings_are_equal(namespaces[i], objectName)) {
                    found = true;
                    break;
                  }

                if (found) {
                  strAppendL(c_code, "_", 1); 
                  break;
                }
              }
                
              si_string_free(objectName);

              strAppendL(c_code, &lexer->token, 1);
              break;
            }

            case '(':
              if (structMode > 1) {
                siString func = si_string_make("");

                bool nb = false;
                int j = 0;

                si_string_append(&func, structName);
                    
                si_string_append_len(&func, "* this", 6);

                stb_lexer lex = *lexer;
                stb_c_lexer_get_token(&lex);

                if (lex.token != ')')
                  si_string_append_len(&func, ", ",  2);


                while ((lexer->token != ';' || nb) && lexer->token != '}') {
                  stb_c_lexer_get_token(lexer);
                  handle_token(lexer, &func);

                  if (!nb && lexer->token == '{') nb = true;
                }

                siArray(siString) split = si_string_split(*c_code, " ");

                si_string_insert(&func, " (", -1);

                si_string_insert(&func, split[si_array_len(split) - 2], 0);
                si_string_insert(&func, "_", 0);
                si_string_insert(&func, structName, 0);
                si_string_insert(&func, "cplus_", 0);
                si_string_insert(&func, " ", 0);
                si_string_insert(&func, split[si_array_len(split) - 3], 0);
                si_string_insert(&func, " ", 0);

                size_t eraseSize = strlen(split[si_array_len(split) - 2]) + strlen(split[si_array_len(split) - 3]);

                for (i = 0; i < si_array_len(split); i++)
                  si_string_free(split[i]);
                
                si_array_free(split); 

                si_string_erase(c_code, si_string_len(*c_code) - eraseSize - 2 - (indent * 2), eraseSize + 2 + (indent * 2));

                si_array_append(&structFuncs, func);
              
                break;
              }

              else if (!si_strings_are_equal(namespace, "") && si_string_len(namespace)) {
                int i;
                for (i = si_string_len(*c_code) - 2; i >= 0 && (*c_code)[i] != ' '; i--);

                si_string_insert(c_code, namespace, i);
                si_string_insert(c_code, "_", i + si_string_len(namespace));
              }

              strAppendL(c_code, &lexer->token, 1);
              break;
            
            case '=':
              structMode = false;

              strAppendL(c_code, &lexer->token, 1);
              break;
            
            case ':': {
              stb_lexer lex = *lexer;
              stb_c_lexer_get_token(&lex);

              if (lex.token == ':' && indent) {
                stb_c_lexer_get_token(lexer);

                lexer->token = '.';
                goto CPLUS_DOT;
                break;
              }

              if (lex.token == ':') {
                stb_c_lexer_get_token(lexer);

                si_string_pop(c_code);

                siString className = si_string_make("");

                int i;
                for (i = si_string_len(*c_code) - 1; i >= 0; i--) {
                  if ((*c_code)[i] == ' ') break;
                  si_string_push(&className, (*c_code)[i]);
                }

                si_string_reverse(&className);

                si_string_insert(c_code, "cplus_", si_string_len(*c_code) - si_string_len(className) - 1);

                si_string_push(c_code, '_');

                bool in = false;

                while (lexer->token != '}') {
                  stb_c_lexer_get_token(lexer);

                  if (in && lexer->token != ')') {
                    in = false;
                    
                    si_string_append(c_code, ", ");
                  }

                  handle_token(lexer, c_code);

                  if (lexer->token == '(') {
                    in = true;

                    si_string_append(c_code, className);
                    si_string_append(c_code, "* this");
                  }
                }

                si_string_free(className);
              }

              else 
                strAppendL(c_code, &lexer->token, 1);
              break;
            }

            default:
              strAppendL(c_code, &lexer->token, 1);
              break;
          }
        }
        else
          printf("<<<UNKNOWN TOKEN %ld >>>\n", lexer->token);
        
        break;
  }

  int i;

  if (si_string_back(*c_code) == '\n' && indent)
    for (i = 0; i < indent * 2; i++) 
      strAppendL(c_code, " ", 1);    
  else if (si_string_back(*c_code) != '\n' && si_string_back(*c_code) != '.' && si_string_back(*c_code) != '_')
      strAppendL(c_code, " ", 1);
}


int main(int argc, char **argv) {
  if (argc == 1) {
    CPLUS_NO_FILE:

    char* error = RED "fatal error" RESET;

    printf("%s%s%s: %s: no input files\ncompilation terminated.\n", 
                BOLD, argv[0], RESET, 
                error);

    return 0;
  }

  bool no_compile = false;
  char* outputName = "output.c";
  char* compiler = "gcc";

  int i;
  siString c_args = si_string_make("");

  siArray(char*) files = si_array_make_reserve(sizeof(char*), 0);

  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (si_strings_are_equal("-cc", argv[i]) && no_compile)
        compiler = argv[i++];

      else if (si_strings_are_equal("-no-compile", argv[i]))
        no_compile = true;
      
      else if (si_strings_are_equal("-o", argv[i]) && no_compile)
        outputName = argv[i++];

      else {
        si_string_push(&c_args, ' ');
        si_string_append(&c_args, argv[i]);
      }
    }

    else if (!si_string_len(c_args)) {
      si_array_append(&files, argv[i]);

      if (!si_path_exists(argv[i])) {
        printf("No such file or directory \"%s\"\n", argv[i]);
        
        exit(1);
      }
    }

    else if (si_string_len(c_args)) {
      si_string_push(&c_args, ' ');
      si_string_append(&c_args, argv[i]);
    }
  }

  if (!si_array_len(files))
    goto CPLUS_NO_FILE;

  siFile f = si_file_open(files[0]); 
  siString text = si_file_read(f); 
  si_file_close(f); 

  stb_lexer lex;
  siString c_code = si_string_make("#define __cplus\n\n");
  
  char* stringStore = malloc(0x10000);
  stb_c_lexer_init(&lex, text, text + f.size, stringStore, 0x10000);

  classes = si_array_make_reserve(sizeof(siString), 0);
  objs = si_array_make_reserve(sizeof(object), 0);
  structFuncs = si_array_make_reserve(sizeof(siString), 0);
  namespaces = si_array_make_reserve(sizeof(char*), 0);

  while (stb_c_lexer_get_token(&lex)) {
      if (lex.token == CLEX_parse_error) {
          printf("\n<<<PARSE ERROR>>>\n");
          break;
      }

      handle_token(&lex, &c_code);
  }

  f = si_file_create(outputName);
  si_file_write(&f, c_code);
  si_file_close(f);

  for (i = 0; i < si_array_len(classes); i++) 
    si_string_free(classes[i]);


  si_array_free(classes);
  si_array_free(objs);
  si_array_free(namespaces);

  si_string_free(c_code);
  free(stringStore);

  siString cmd = si_string_make(compiler);
  si_string_push(&cmd, ' ');
  si_string_append(&cmd, outputName);

  si_string_append(&cmd, c_args);

  if (!no_compile) {
    system(cmd);
    si_path_remove("output.c");
  }

  si_string_free(cmd);

  si_string_free(c_args);
  si_array_free(files);
}
