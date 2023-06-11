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

#ifdef MEMWATCH
#include "memwatch/memwatch.h"
#endif

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

siArray(siString) structFuncs = NULL; /* functions defined inside the struct */

typedef struct object { 
    char* varName;
    char* varType; 
    unsigned int indent; 
} object;

siArray(siString) classes;
siArray(object) objs;

siString* handle_token(stb_lexer *lexer, siString* c_code) {
  switch (lexer->token) {
    case CLEX_id        :   
              if (structMode == 1)
                structName = si_string_make(lexer->string);
              
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
    case CLEX_arrow     : strAppendL(c_code, "->", 2); break;
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
                strAppendL(c_code, &lexer->token, 1);
                strAppendL(c_code, "\n", 1);
                break;
            
            case '}':
              if (structMode)
                structMode--;
                
              indent--;

              si_string_erase(c_code, si_string_len(*c_code) - 2, 2);
              strAppendL(c_code, &lexer->token, 1);
            

              if (structMode == 1 && typedefCheck)
                strAppendL(c_code, " ", 1);

              strAppendL(c_code, "\n\n", 2);
              break;

            case ';':
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
                    
                    bool next = false;
                    for (j = 0; j < si_string_len(structFuncs[i]); j++) {
                      if (next == false) {
                        si_string_erase(&structFuncs[i], j, 2);
                        next = true;
                      }

                      if (structFuncs[i][j] == '\n')
                        next = false;
                    }
                    si_string_append(c_code, structFuncs[i]);
                    si_string_append(c_code, "\n");

                    si_string_free(structFuncs[i]);
                  }

                  si_array_free(structFuncs);
                  structMode = 0;
                }
                break;

            case '.': {
              if (si_string_back(*c_code) == ' ')
                si_string_erase(c_code, si_string_len(*c_code) - 1, 1);
              
              stb_lexer lex;
              char* funcName;

              for (lex = *lexer; lex.token != ';' && lex.token != '('; stb_c_lexer_get_token(&lex)) 
                funcName = lex.string;


              if (lex.token == '(') {
                siString func = si_string_make("cplus_"); 

                siString objectName = si_string_make("");

                int i;
                for (i = si_string_len(*c_code) - 1; i >= 0; i--) {
                  if ((*c_code)[i] == ' ') break;
                  si_string_push(&objectName, (*c_code)[i]);
                }

                si_string_reverse(&objectName);
              
                int xx = 0; 

                for (i = 0; i < si_array_len(objs); i++) {
                  if (si_strings_are_equal(objs[i].varName, objectName)) 
                    if (objs[i].indent <= indent && 
                        objs[i].indent > objs[xx].indent)
                      xx = i;
                }

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
                si_string_free(objectName);
              }

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

                if (structFuncs == NULL)
                  structFuncs = si_array_make_reserve(sizeof(siString), 1);
                  
                si_array_append(&structFuncs, func);
              }

              else 
                strAppendL(c_code, &lexer->token, 1);
              break;
            
            case '=':
              structMode = false;

              strAppendL(c_code, &lexer->token, 1);
              break;
            
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
  else if (si_string_back(*c_code) != '\n' && si_string_back(*c_code) != '.')
      strAppendL(c_code, " ", 1);

}

int main(int argc, char **argv) {
  if (argc == 1) {
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
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (si_strings_are_equal("-cc", argv[i]) && no_compile)
        compiler = argv[i++];

      if (si_strings_are_equal("-no-compile", argv[i]))
        no_compile == true;
      
      if (si_strings_are_equal("-o", argv[i]) && no_compile)
        outputName = argv[i++];
    }
  }

  siFile f = si_file_open("test.cp");
  siString text = si_file_read(f);
  si_file_close(f);

  stb_lexer lex;
  siString c_code = si_string_make("#define __cplus\n\n");
  
  char* stringStore = malloc(0x10000);
  stb_c_lexer_init(&lex, text, text + f.size, stringStore, 0x10000);

  classes = si_array_make_reserve(sizeof(siString), 0);
  objs = si_array_make_reserve(sizeof(object), 0);

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


  for (int i = 0; i < si_array_len(classes); i++) 
    si_string_free(classes[i]);


  si_array_free(classes);
  si_array_free(objs);

  si_string_free(c_code);
  free(stringStore);

  siString cmd = si_string_make(compiler);
  si_string_push(&cmd, ' ');
  si_string_append(&cmd, outputName);

  system(cmd);

  si_string_free(cmd);
}
