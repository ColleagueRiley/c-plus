#define STB_C_LEXER_IMPLEMENTATION
#define SI_IMPLEMENTATION

#include "sili.h"

#include "stb_c_lexer.h"

#include <assert.h>


siString* print_token(stb_lexer *lexer, siString* c_code) {
  switch (lexer->token) {
    case CLEX_id        : si_string_append(c_code, lexer->string); break;
    case CLEX_eq        : si_string_append(c_code, "=="); break;
    case CLEX_noteq     : si_string_append(c_code, "!="); break;
    case CLEX_lesseq    : si_string_append(c_code, "<="); break;
    case CLEX_greatereq : si_string_append(c_code, ">="); break;
    case CLEX_andand    : si_string_append(c_code, "&&"); break;
    case CLEX_oror      : si_string_append(c_code, "||"); break;
    case CLEX_shl       : si_string_append(c_code, "<<"); break;
    case CLEX_shr       : si_string_append(c_code, ">>"); break;
    case CLEX_plusplus  : si_string_append(c_code, "++"); break;
    case CLEX_minusminus: si_string_append(c_code, "--"); break;
    case CLEX_arrow     : si_string_append(c_code, "->"); break;
    case CLEX_andeq     : si_string_append(c_code, "&="); break;
    case CLEX_oreq      : si_string_append(c_code, "|="); break;
    case CLEX_xoreq     : si_string_append(c_code, "^="); break;
    case CLEX_pluseq    : si_string_append(c_code, "+="); break;
    case CLEX_minuseq   : si_string_append(c_code, "-="); break;
    case CLEX_muleq     : si_string_append(c_code, "*="); break;
    case CLEX_diveq     : si_string_append(c_code, "/="); break;
    case CLEX_modeq     : si_string_append(c_code, "%%="); break;
    case CLEX_shleq     : si_string_append(c_code, "<<="); break;
    case CLEX_shreq     : si_string_append(c_code, ">>="); break;
    case CLEX_eqarrow   : si_string_append(c_code, "=>"); break;
    case CLEX_dqstring  : 
          si_string_append(c_code, "\""); 
          si_string_append(c_code, lexer->string); 
          si_string_append(c_code, "\""); 
          
          break;
    case CLEX_sqstring  : 
          si_string_append(c_code, "\"'"); 
          si_string_append(c_code, lexer->string); 
          si_string_append(c_code, "\"'"); 
    case CLEX_charlit   : 
          si_string_append(c_code, "'"); 
          si_string_append(c_code, lexer->string); 
          si_string_append(c_code, "'"); 
    #if defined(STB__clex_int_as_double) && !defined(STB__CLEX_use_stdlib)
    case CLEX_intlit    : si_string_append(c_code, "#%g", lexer->real_number); break;
    #else
    case CLEX_intlit    : 
      //si_string_append(c_code, lexer->int_number); break;
    #endif
    case CLEX_floatlit  : 
      //si_string_append(c_code, lexer->real_number); break;
    default:
        if (lexer->token >= 0 && lexer->token < 256) {
          switch(lexer->token) {
            case '{':
                si_string_append(c_code, &lexer->token);
                si_string_append(c_code, "\n");
                break;
            
            case '}':
              si_string_append(c_code, &lexer->token);
              si_string_append(c_code, "\n\n");
              break;

            case ';':
                if (si_string_back(c_code) == '\n')
                  si_string_erase(c_code, si_string_len(c_code) - 1, si_string_len(c_code));

                si_string_append(c_code, &lexer->token);
                si_string_append(c_code, "\n");
                break;
            
            default:
              si_string_append(c_code, &lexer->token);
              break;
          }
        }
        else
          printf("<<<UNKNOWN TOKEN %ld >>>\n", lexer->token);
        
        break;
  }

  si_string_append(c_code, " ");
}

int main(int argc, char **argv) {
  siFile f = si_file_open_mode("test.cp", SI_FILE_MODE_READ);
  siString text = si_file_read(f);
  si_file_close(f);

  stb_lexer lex;
  siString c_code = si_string_make("");
  stb_c_lexer_init(&lex, text, text + f.size, malloc(0x10000), 0x10000);

  while (stb_c_lexer_get_token(&lex)) {
      if (lex.token == CLEX_parse_error) {
          printf("\n<<<PARSE ERROR>>>\n");
          break;
      }

      print_token(&lex, &c_code);
  }

  f = si_file_create("output.c");
  si_file_write(&f, c_code);
  si_file_close(f);

  si_string_free(c_code);
}