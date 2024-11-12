#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "common.h"
#include "../phase.h"

enum { LINE_OFFSET = 1 };
enum { ACC_EOF_CHARACTER = 127 };

struct request {
   const char* given_path;
   struct file_entry* file;
   struct file_entry* offset_file;
   struct source* source;
   bool err_open;
   bool err_loading;
   bool err_loaded_before;
   bool implicit_bcs_ext;
};

static void append_file( struct library* lib, struct file_entry* file );
static void init_request( struct request* request,
   struct file_entry* offset_file, const char* path );
static void init_request_module( struct request* request,
   struct file_entry* file );
static void check_implicit_ext( struct parse* parse, struct request* request );
static void load_source( struct parse* parse, struct request* request );
static void find_source( struct parse* parse, struct request* request );
static bool source_loading( struct parse* parse, struct request* request );
static void load_module( struct parse* parse, struct request* request );
static void open_source_file( struct parse* parse, struct request* request );
static struct source* alloc_source( struct parse* parse );
static void reset_filepos( struct source* source );
static void create_entry( struct parse* parse, struct request* request,
   bool imported );
static void create_include_history_entry( struct parse* parse, int line );
static void create_include_history_entry_imported( struct parse* parse,
   struct import_dirc* dirc );
static void escape_ch( struct parse* parse, char*, struct str* text, bool );
static char read_ch( struct parse* parse );
static char peek_ch( struct parse* parse );
static void read_initial_ch( struct parse* parse );
static struct str* temp_text( struct parse* parse );
static void append_ch( struct str* str, char ch );
static void append_string_ch( struct str* text, char ch );

void p_load_main_source( struct parse* parse ) {
   struct request request;
   init_request( &request, NULL, parse->task->options->source_file );
   load_source( parse, &request );
   if ( request.source ) {
      parse->lib->file = request.file;
      parse->lib->file_pos.id = request.file->id;
      append_file( parse->lib, request.file );
      create_entry( parse, &request, false );
      create_include_history_entry( parse, 0 );
      t_update_err_file_dir( parse->task, request.file->full_path.value );
   }
   else {
      p_diag( parse, DIAG_ERR,
         "failed to load source file: \"%s\" (%s)",
         parse->task->options->source_file, strerror( errno ) );
      p_bail( parse );
   }
}

void p_load_imported_lib_source( struct parse* parse, struct import_dirc* dirc,
   struct file_entry* file ) {
   struct request request;
   init_request_module( &request, file );
   load_module( parse, &request );
   if ( request.source ) {
      parse->lib->file = file;
      parse->lib->file_pos.id = file->id;
      append_file( parse->lib, file );
      create_entry( parse, &request, true );
      create_include_history_entry_imported( parse, dirc );
   }
   else {
      p_diag( parse, DIAG_POS_ERR, &dirc->pos,
         "failed to load library file: \"%s\"", dirc->file_path );
      p_bail( parse );
   }
}

struct file_entry* p_find_module_file( struct parse* parse,
   struct library* importing_module, const char* path ) {
   struct request request;
   init_request( &request, importing_module->file, path );
   check_implicit_ext( parse, &request );
   find_source( parse, &request );
   return request.file;
}

void p_load_included_source( struct parse* parse, const char* file_path,
   struct pos* pos ) {
   struct request request;
   init_request( &request, parse->source->file, file_path );
   check_implicit_ext( parse, &request );
   load_source( parse, &request );
   if ( request.source ) {
      append_file( parse->lib, request.file );
      create_entry( parse, &request, false );
      create_include_history_entry( parse, pos->line );
      p_define_included_macro( parse );
   }
   else {
      if ( request.err_loading ) {
         p_diag( parse, DIAG_POS_ERR, pos,
            "file already being loaded" );
         p_bail( parse );
      }
      else {
         p_diag( parse, DIAG_POS_ERR, pos,
            "failed to load file: \"%s\"", file_path );
         p_bail( parse );
      }
   }
   parse->source_entry->prev_tk = TK_NL;
   // A common mistake is for the user to #include the zcommon.acs file
   // multiple times. Error out when this happens.
   if ( strcmp( file_path, "zcommon.acs" ) == 0 ) {
      if ( ! parse->zcommon.included ) {
         parse->zcommon.pos = *pos;
         parse->zcommon.included = true;
      }
      else {
         p_diag( parse, DIAG_POS_ERR, pos,
            "#including zcommon.acs file multiple times" );
         p_diag( parse, DIAG_POS | DIAG_NOTE, &parse->zcommon.pos,
            "zcommon.acs file first #included here" );
         p_bail( parse );
      }
   }
}

static void append_file( struct library* lib, struct file_entry* file ) {
   struct list_iter i;
   list_iterate( &lib->files, &i );
   while ( ! list_end( &i ) ) {
      if ( list_data( &i ) == file ) {
         return;
      }
      list_next( &i );
   }
   list_append( &lib->files, file );
}

static void init_request( struct request* request,
   struct file_entry* offset_file, const char* path ) {
   request->given_path = path;
   request->file = NULL;
   request->offset_file = offset_file;
   request->source = NULL;
   request->err_open = false;
   request->err_loaded_before = false;
   request->err_loading = false;
   request->implicit_bcs_ext = false;
}

static void init_request_module( struct request* request,
   struct file_entry* file ) {
   init_request( request, NULL, file->full_path.value );
   request->file = file;
}

static void check_implicit_ext( struct parse* parse, struct request* request ) {
   if ( bcc_stricmp( c_get_file_ext( request->given_path ), "h" ) == 0 ) {
      request->implicit_bcs_ext = true;
   }
}

static void load_source( struct parse* parse, struct request* request ) {
   find_source( parse, request );
   if ( request->file ) {
      if ( ! source_loading( parse, request ) ) {
         open_source_file( parse, request );
      }
      else {
         request->err_loading = true;
      }
   }
   else {
      request->err_open = true;
   }
}

static void find_source( struct parse* parse, struct request* request ) {
   if ( request->implicit_bcs_ext ) {
      struct str path;
      str_init( &path );
      str_append( &path, request->given_path );
      str_append( &path, ".bcs" );
      struct file_query query;
      t_init_file_query( &query,
         request->offset_file,
         path.value );
      t_find_file( parse->task, &query );
      if ( query.success ) {
         request->file = query.file;
      }
      str_deinit( &path );
      if ( request->source ) {
         return;
      }
   }
   struct file_query query;
   t_init_file_query( &query,
      request->offset_file,
      request->given_path );
   t_find_file( parse->task, &query );
   if ( query.success ) {
      request->file = query.file;
   }
}

static bool source_loading( struct parse* parse, struct request* request ) {
   struct source_entry* entry = parse->source_entry;
   while ( entry && ( ! entry->source ||
      request->file != entry->source->file ) ) {
      entry = entry->prev;
   }
   return ( entry != NULL );
}

static void load_module( struct parse* parse, struct request* request ) {
   open_source_file( parse, request );
}

static void open_source_file( struct parse* parse, struct request* request ) {
   FILE* fh = fopen( request->file->full_path.value, "rb" );
   if ( ! fh ) {
      request->err_open = true;
      return;
   }
   // Create source.
   struct source* source = alloc_source( parse );
   source->file = request->file;
   source->file_entry_id = source->file->id;
   source->fh = fh;
   source->prev = NULL;
   request->source = source;
}

static struct source* alloc_source( struct parse* parse ) {
   // Allocate.
   struct source* source;
   if ( parse->free_source ) {
      source = parse->free_source;
      parse->free_source = source->prev;
   }
   else {
      source = mem_alloc( sizeof( *source ) );
   }
   // Initialize with default values.
   source->file = NULL;
   source->fh = NULL;
   source->prev = NULL;
   reset_filepos( source );
   source->ch = '\0';
   source->buffer_pos = SOURCE_BUFFER_SIZE;
   return source;
}

static void reset_filepos( struct source* source ) {
   source->line = LINE_OFFSET;
   source->column = 0;
}

static void create_entry( struct parse* parse, struct request* request,
   bool imported ) {
   struct source_entry* entry;
   if ( parse->source_entry_free ) {
      entry = parse->source_entry_free;
      parse->source_entry_free = entry->prev;
   }
   else {
      entry = mem_alloc( sizeof( *entry ) );
   }
   entry->prev = parse->source_entry;
   entry->source = request->source;
   entry->macro_expan = NULL;
   p_init_token_queue( &entry->peeked, false );
   entry->main = ( entry->prev == NULL );
   entry->imported = imported;
   entry->prev_tk = TK_NL;
   entry->line_beginning = true;
   parse->source_entry = entry;
   parse->source = entry->source;
   parse->macro_expan = NULL;
   parse->tkque = &entry->peeked;
   parse->tk = TK_END;
   read_initial_ch( parse );
}

static void create_include_history_entry( struct parse* parse, int line ) {
   struct include_history_entry* entry =
      t_alloc_include_history_entry( parse->task );
   entry->parent = parse->include_history_entry;
   entry->file_entry_id = parse->source->file_entry_id;
   entry->line = line;
   parse->include_history_entry = entry;
   parse->source->file_entry_id = entry->id;
}

static void create_include_history_entry_imported( struct parse* parse,
   struct import_dirc* dirc ) {
   struct include_history_entry* entry =
      t_alloc_include_history_entry( parse->task );
   entry->parent = t_decode_include_history_entry( parse->task, dirc->pos.id );
   entry->file_entry_id = parse->source->file_entry_id;
   entry->line = dirc->pos.line;
   entry->imported = true;
   parse->include_history_entry = entry;
   parse->source->file_entry_id = entry->id;
}

void p_add_altern_file_name( struct parse* parse,
   const char* name, int line ) {
   struct include_history_entry* entry =
      t_alloc_include_history_entry( parse->task );
   entry->parent = parse->include_history_entry->parent;
   entry->altern_name = name;
   entry->line = parse->include_history_entry->line;
   entry->imported = parse->include_history_entry->imported;
   parse->include_history_entry = entry;
   parse->source->file_entry_id = entry->id;
   parse->source->line = line;
}

void p_pop_source( struct parse* parse ) {
   struct source_entry* entry = parse->source_entry;
   struct source* source = entry->source;
   fclose( source->fh );
   if ( entry->main ) {
      parse->main_lib_lines = source->line - LINE_OFFSET;
   }
   else {
      parse->included_lines += source->line - LINE_OFFSET;
   }
   source->prev = parse->free_source;
   parse->free_source = source;
   entry->source = NULL;
   if ( entry->prev ) {
      // Load previous entry.
      parse->source_entry = entry->prev;
      parse->source = parse->source_entry->source;
      parse->macro_expan = parse->source_entry->macro_expan;
      parse->tkque = &parse->source_entry->peeked;
      // Free entry.
      entry->prev = parse->source_entry_free;
      parse->source_entry_free = entry;
      // We are now back to the library file. Remove the __INCLUDED__ macro.
      if ( parse->source_entry->main ||
         parse->source_entry->imported ) {
         p_undefine_included_macro( parse );
      }
   }
   // Include history.
   if ( parse->include_history_entry ) {
      parse->include_history_entry = parse->include_history_entry->parent;
   }
}

void p_read_source( struct parse* parse, struct token* token ) {
   char ch = parse->source->ch;
   int line = 0;
   int column = 0;
   int length = 0;
   enum tk tk = TK_END;
   struct str* text = NULL;

   whitespace:
   // -----------------------------------------------------------------------
   switch ( ch ) {
   case ' ':
   case '\t':
      goto spacetab;
   case '\n':
      goto newline;
   default:
      goto graph;
   }

   spacetab:
   // -----------------------------------------------------------------------
   line = parse->source->line;
   column = parse->source->column;
   while ( ch == ' ' || ch == '\t' ) {
      ch = read_ch( parse );
   }
   length = parse->source->column - column;
   tk = TK_HORZSPACE;
   goto finish;

   newline:
   // -----------------------------------------------------------------------
   line = parse->source->line;
   column = parse->source->column;
   tk = TK_NL;
   ch = read_ch( parse );
   goto finish;

   // The chain of if-statements is ordered based on a likelihood of a token
   // being used. Identifier tokens are one of the most common, so look for
   // them first.
   graph:
   // -----------------------------------------------------------------------
   line = parse->source->line;
   column = parse->source->column;
   if ( isalpha( ch ) || ch == '_' ) {
      goto identifier;
   }
   else if ( ch == '(' ) {
      tk = TK_PAREN_L;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == ')' ) {
      tk = TK_PAREN_R;
      read_ch( parse );
      goto finish;
   }
   else if ( isdigit( ch ) ) {
      if ( ch == '0' ) {
         ch = read_ch( parse );
         switch ( ch ) {
         case 'b':
         case 'B':
            goto binary;
         case 'x':
         case 'X':
            goto hexadecimal;
         case 'o':
         case 'O':
            ch = read_ch( parse );
            goto octal;
         case '.':
            text = temp_text( parse );
            append_ch( text, '0' );
            append_ch( text, '.' );
            ch = read_ch( parse );
            goto fixedpoint;
         default:
            goto zero;
         }
      }
      else {
         goto decimal;
      }
   }
   else if ( ch == ',' ) {
      tk = TK_COMMA;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == ';' ) {
      tk = TK_SEMICOLON;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == '"' ) {
      ch = read_ch( parse );
      goto string;
   }
   else if ( ch == ':' ) {
      ch = read_ch( parse );
      if ( ch == ':' ) {
         tk = TK_COLONCOLON;
         read_ch( parse );
      }
      else {
         tk = TK_COLON;
      }
      goto finish;
   }
   else if ( ch == '#' ) {
      ch = read_ch( parse );
      if ( ch == '#' ) {
         tk = TK_HASHHASH;
         ch = read_ch( parse );
      }
      else {
         tk = TK_HASH;
      }
      goto finish;
   }
   else if ( ch == '{' ) {
      tk = TK_BRACE_L;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == '}' ) {
      tk = TK_BRACE_R;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == '=' ) {
      ch = read_ch( parse );
      if ( ch == '=' ) {
         tk = TK_EQ;
         ch = read_ch( parse );
      }
      else {
         tk = TK_ASSIGN;
      }
      goto finish;
   }
   else if ( ch == '[' ) {
      tk = TK_BRACKET_L;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == ']' ) {
      tk = TK_BRACKET_R;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == '.' ) {
      ch = read_ch( parse );
      if ( ch == '.' && peek_ch( parse ) == '.' ) {
         read_ch( parse );
         ch = read_ch( parse );
         tk = TK_ELLIPSIS;
      }
      else {
         tk = TK_DOT;
      }
      goto finish;
   }
   else if ( ch == '+' ) {
      ch = read_ch( parse );
      if ( ch == '+' ) {
         tk = TK_INC;
         ch = read_ch( parse );
      }
      else if ( ch == '=' ) {
         tk = TK_ASSIGN_ADD;
         ch = read_ch( parse );
      }
      else {
         tk = TK_PLUS;
      }
      goto finish;
   }
   else if ( ch == '-' ) {
      ch = read_ch( parse );
      if ( ch == '-' ) {
         tk = TK_DEC;
         ch = read_ch( parse );
      }
      else if ( ch == '=' ) {
         tk = TK_ASSIGN_SUB;
         ch = read_ch( parse );
      }
      else {
         tk = TK_MINUS;
      }
      goto finish;
   }
   else if ( ch == '!' ) {
      ch = read_ch( parse );
      if ( ch == '=' ) {
         tk = TK_NEQ;
         ch = read_ch( parse );
      }
      else {
         tk = TK_LOG_NOT;
      }
      goto finish;
   }
   else if ( ch == '&' ) {
      ch = read_ch( parse );
      if ( ch == '&' ) {
         tk = TK_LOG_AND;
         ch = read_ch( parse );
      }
      else if ( ch == '=' ) {
         tk = TK_ASSIGN_BIT_AND;
         ch = read_ch( parse );
      }
      else {
         tk = TK_BIT_AND;
      }
      goto finish;
   }
   else if ( ch == '<' ) {
      ch = read_ch( parse );
      if ( ch == '=' ) {
         tk = TK_LTE;
         ch = read_ch( parse );
      }
      else if ( ch == '<' ) {
         ch = read_ch( parse );
         if ( ch == '=' ) {
            tk = TK_ASSIGN_SHIFT_L;
            ch = read_ch( parse );
         }
         else {
            tk = TK_SHIFT_L;
         }
      }
      else {
         tk = TK_LT;
      }
      goto finish;
   }
   else if ( ch == '>' ) {
      ch = read_ch( parse );
      if ( ch == '=' ) {
         tk = TK_GTE;
         ch = read_ch( parse );
      }
      else if ( ch == '>' ) {
         ch = read_ch( parse );
         if ( ch == '=' ) {
            tk = TK_ASSIGN_SHIFT_R;
            ch = read_ch( parse );
            goto finish;
         }
         else {
            tk = TK_SHIFT_R;
            goto finish;
         }
      }
      else {
         tk = TK_GT;
      }
      goto finish;
   }
   else if ( ch == '|' ) {
      ch = read_ch( parse );
      if ( ch == '|' ) {
         tk = TK_LOG_OR;
         ch = read_ch( parse );
      }
      else if ( ch == '=' ) {
         tk = TK_ASSIGN_BIT_OR;
         ch = read_ch( parse );
      }
      else {
         tk = TK_BIT_OR;
      }
      goto finish;
   }
   else if ( ch == '*' ) {
      ch = read_ch( parse );
      if ( ch == '=' ) {
         tk = TK_ASSIGN_MUL;
         ch = read_ch( parse );
      }
      else {
         tk = TK_STAR;
      }
      goto finish;
   }
   else if ( ch == '/' ) {
      ch = read_ch( parse );
      if ( ch == '=' ) {
         tk = TK_ASSIGN_DIV;
         ch = read_ch( parse );
         goto finish;
      }
      else if ( ch == '/' ) {
         goto comment;
      }
      else if ( ch == '*' ) {
         ch = read_ch( parse );
         goto multiline_comment;
      }
      else {
         tk = TK_SLASH;
         goto finish;
      }
   }
   else if ( ch == '%' ) {
      ch = read_ch( parse );
      if ( ch == '=' ) {
         tk = TK_ASSIGN_MOD;
         ch = read_ch( parse );
      }
      else {
         tk = TK_MOD;
      }
      goto finish;
   }
   else if ( ch == '^' ) {
      ch = read_ch( parse );
      if ( ch == '=' ) {
         tk = TK_ASSIGN_BIT_XOR;
         ch = read_ch( parse );
      }
      else {
         tk = TK_BIT_XOR;
      }
      goto finish;
   }
   else if ( ch == '\'' ) {
      ch = read_ch( parse );
      goto character;
   }
   else if ( ch == '~' ) {
      tk = TK_BIT_NOT;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == '?' ) {
      tk = TK_QUESTION_MARK;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == '@' ) {
      tk = TK_AT;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == '\\' ) {
      tk = TK_BACKSLASH;
      read_ch( parse );
      goto finish;
   }
   else if ( ch == '\0' ) {
      tk = TK_END;
      goto finish;
   }
   else {
      struct pos pos;
      t_init_pos( &pos,
         parse->source->file_entry_id,
         parse->source->line,
         column );
      p_diag( parse, DIAG_POS_ERR, &pos,
         "invalid character" );
      p_bail( parse );
   }

   identifier:
   // -----------------------------------------------------------------------
   {
      int length = 0;
      text = temp_text( parse );
      while ( isalnum( ch ) || ch == '_' ) {
         append_ch( text, ch );
         ch = read_ch( parse );
         ++length;
      }
      if ( strcmp( text->value, "__VA_ARGS__" ) == 0 &&
         ! parse->variadic_macro_context ) {
         struct pos pos;
         t_init_pos( &pos,
            parse->source->file_entry_id,
            line, column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "`__VA_ARGS__` can only appear in the body of a variadic macro" );
         p_bail( parse );
      }
      tk = TK_ID;
      goto finish;
   }

   binary:
   // -----------------------------------------------------------------------
   text = temp_text( parse );
   ch = read_ch( parse );
   while ( true ) {
      if ( ch == '0' || ch == '1' ) {
         append_ch( text, ch );
         ch = read_ch( parse );
      }
      // Single quotation marks can be used to improve the readability of long
      // numeric literals by visually grouping digits. Such a single quotation
      // mark is called a digit separator. Digit separators are ignored by the
      // compiler.
      else if ( ch == '\'' ) {
         ch = read_ch( parse );
         if ( ! ( ch == '0' || ch == '1' ) ) {
            struct pos pos;
            t_init_pos( &pos,
               parse->source->file_entry_id,
               parse->source->line,
               parse->source->column );
            p_diag( parse, DIAG_POS_ERR, &pos,
               "missing binary digit after digit separator" );
            p_bail( parse );
         }
      }
      else if ( isalnum( ch ) ) {
         struct pos pos;
         t_init_pos( &pos,
            parse->source->file_entry_id,
            parse->source->line,
            parse->source->column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "invalid digit in binary literal" );
         p_bail( parse );
      }
      else if ( text->length == 0 ) {
         struct pos pos;
         t_init_pos( &pos,
            parse->source->file_entry_id,
            parse->source->line,
            column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "binary literal has no digits" );
         p_bail( parse );
      }
      else {
         tk = TK_LIT_BINARY;
         goto finish;
      }
   }

   hexadecimal:
   // -----------------------------------------------------------------------
   text = temp_text( parse );
   ch = read_ch( parse );
   while ( true ) {
      if ( isxdigit( ch ) ) {
         append_ch( text, ch );
         ch = read_ch( parse );
      }
      else if ( ch == '\'' ) {
         ch = read_ch( parse );
         if ( ! isxdigit( ch ) ) {
            struct pos pos;
            t_init_pos( &pos,
               parse->source->file_entry_id,
               parse->source->line,
               parse->source->column );
            p_diag( parse, DIAG_POS_ERR, &pos,
               "missing hexadecimal digit after digit separator" );
            p_bail( parse );
         }
      }
      else if ( isalnum( ch ) ) {
         struct pos pos;
         t_init_pos( &pos,
            parse->source->file_entry_id,
            parse->source->line,
            parse->source->column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "invalid digit in hexadecimal literal" );
         p_bail( parse );
      }
      else {
         if ( text->length == 0 ) {
            struct pos pos;
            t_init_pos( &pos,
               parse->source->file_entry_id,
               parse->source->line,
               column );
            p_diag( parse, DIAG_POS | DIAG_WARN, &pos,
               "hexadecimal literal has no digits, will interpret it as 0x0" );
            append_ch( text, '0' );
         }
         tk = TK_LIT_HEX;
         goto finish;
      }
   }

   octal:
   // -----------------------------------------------------------------------
   text = temp_text( parse );
   while ( true ) {
      if ( ch >= '0' && ch <= '7' ) {
         append_ch( text, ch );
         ch = read_ch( parse );
      }
      else if ( ch == '\'' ) {
         ch = read_ch( parse );
         if ( ! ( ch >= '0' && ch <= '7' ) ) {
            struct pos pos;
            t_init_pos( &pos,
               parse->source->file_entry_id,
               parse->source->line,
               parse->source->column );
            p_diag( parse, DIAG_POS_ERR, &pos,
               "missing octal digit after digit separator" );
            p_bail( parse );
         }
      }
      else if ( isalnum( ch ) ) {
         struct pos pos;
         t_init_pos( &pos,
            parse->source->file_entry_id,
            parse->source->line,
            parse->source->column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "invalid digit in octal literal" );
         p_bail( parse );
      }
      else {
         if ( text->length == 0 ) {
            struct pos pos;
            t_init_pos( &pos,
               parse->source->file_entry_id,
               parse->source->line,
               column );
            p_diag( parse, DIAG_POS_ERR, &pos,
               "octal literal has no digits" );
            p_bail( parse );
         }
         tk = TK_LIT_OCTAL;
         goto finish;
      }
   }

   zero:
   // -----------------------------------------------------------------------
   while ( ch == '0' || ( ch == '\'' && peek_ch( parse ) == '0' ) ) {
      ch = read_ch( parse );
   }
   if ( isdigit( ch ) || ch == '\'' ) {
      goto decimal;
   }
   else if ( ch == '.' ) {
      text = temp_text( parse );
      append_ch( text, '0' );
      append_ch( text, '.' );
      ch = read_ch( parse );
      goto fixedpoint;
   }
   else if ( ch == 'r' || ch == 'R' || ch == '_' ) {
      text = temp_text( parse );
      append_ch( text, '0' );
      append_ch( text, tolower( ch ) );
      ch = read_ch( parse );
      goto radix;
   }
   else {
      text = temp_text( parse );
      str_append( text, "0" );
      tk = TK_LIT_DECIMAL;
      goto finish;
   }

   decimal:
   // -----------------------------------------------------------------------
   text = temp_text( parse );
   while ( true ) {
      if ( isdigit( ch ) ) {
         append_ch( text, ch );
         ch = read_ch( parse );
      }
      else if ( ch == '\'' ) {
         ch = read_ch( parse );
         if ( ! isdigit( ch ) ) {
            struct pos pos;
            t_init_pos( &pos,
               parse->source->file_entry_id,
               parse->source->line,
               parse->source->column );
            p_diag( parse, DIAG_POS_ERR, &pos,
               "missing decimal digit after digit separator" );
            p_bail( parse );
         }
      }
      // Fixed-point number.
      else if ( ch == '.' ) {
         append_ch( text, ch );
         ch = read_ch( parse );
         goto fixedpoint;
      }
      // In ACS, an underscore is used for the separator between the base and
      // the value of a radix constant. In BCS, a single quotation mark is used
      // for the digit separator. When both these characters appear in a radix
      // constant, it might look confusing. To improve readability, allow 'r'
      // and 'R' to substitute for the underscore.
      else if ( ch == 'r' || ch == 'R' || ch == '_' ) {
         append_ch( text, tolower( ch ) );
         ch = read_ch( parse );
         goto radix;
      }
      else if ( isalpha( ch ) ) {
         struct pos pos;
         t_init_pos( &pos,
            parse->source->file_entry_id,
            parse->source->line,
            parse->source->column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "invalid digit in decimal literal" );
         p_bail( parse );
      }
      else {
         tk = TK_LIT_DECIMAL;
         goto finish;
      }
   }

   fixedpoint:
   // -----------------------------------------------------------------------
   while ( true ) {
      if ( isdigit( ch ) ) {
         append_ch( text, ch );
         ch = read_ch( parse );
      }
      else if ( ch == '\'' ) {
         ch = read_ch( parse );
         if ( ! isdigit( ch ) ) {
            struct pos pos;
            t_init_pos( &pos,
               parse->source->file_entry_id,
               parse->source->line,
               parse->source->column );
            p_diag( parse, DIAG_POS_ERR, &pos,
               "missing decimal digit after digit separator" );
            p_bail( parse );
         }
      }
      else if ( isalpha( ch ) ) {
         struct pos pos;
         t_init_pos( &pos,
            parse->source->file_entry_id,
            parse->source->line,
            parse->source->column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "invalid digit in fractional part of fixed-point literal" );
         p_bail( parse );
      }
      else {
         if ( text->value[ text->length - 1 ] == '.' ) {
            struct pos pos;
            t_init_pos( &pos,
               parse->source->file_entry_id,
               parse->source->line,
               column );
            p_diag( parse, DIAG_POS | DIAG_WARN, &pos,
               "fixed-point literal has no digits after point, will interpret "
               "it as %s0", text->value );
            append_ch( text, '0' );
         }
         tk = TK_LIT_FIXED;
         goto finish;
      }
   }

   radix:
   // -----------------------------------------------------------------------
   if ( ! ( isalnum( ch ) || ch == '\'' ) ) {
      struct pos pos;
      t_init_pos( &pos,
         parse->source->file_entry_id,
         parse->source->line,
         column );
      p_diag( parse, DIAG_POS | DIAG_WARN, &pos,
         "radix literal has no digits after %s, will interpret it as %s0",
         ( text->value[ text->length - 1 ] == 'r' ) ? "'r'" : "underscore",
         text->value );
   }
   text->value[ text->length - 1 ] = '_';
   while ( true ) {
      if ( isalnum( ch ) ) {
         append_ch( text, tolower( ch ) );
         ch = read_ch( parse );
      }
      else if ( ch == '\'' ) {
         ch = read_ch( parse );
         if ( ! isalnum( ch ) ) {
            struct pos pos;
            t_init_pos( &pos,
               parse->source->file_entry_id,
               parse->source->line,
               parse->source->column );
            p_diag( parse, DIAG_POS_ERR, &pos,
               "missing digit after digit separator" );
            p_bail( parse );
         }
      }
      else {
         if ( text->value[ text->length - 1 ] == '_' ) {
            append_ch( text, '0' );
         }
         tk = TK_LIT_RADIX;
         goto finish;
      }
   }

   string:
   // -----------------------------------------------------------------------
   text = temp_text( parse );
   while ( true ) {
      if ( ! ch ) {
         struct pos pos;
         t_init_pos( &pos,
            parse->source->file_entry_id,
            line, column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "unterminated string" );
         p_bail( parse );
      }
      else if ( ch == ACC_EOF_CHARACTER ) {
         struct pos pos;
         t_init_pos( &pos,
            parse->source->file_entry_id,
            parse->source->line,
            parse->source->column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "invalid character in string literal" );
         p_bail( parse );
      }
      else if ( ch == '"' ) {
         ch = read_ch( parse );
         tk = TK_LIT_STRING;
         goto finish;
      }
      else if ( ch == '\\' ) {
         append_string_ch( text, ch );
         ch = read_ch( parse );
         if ( ch ) {
            append_string_ch( text, ch );
            ch = read_ch( parse );
         }
      }
      else {
         append_string_ch( text, ch );
         ch = read_ch( parse );
      }
   }

   character:
   // -----------------------------------------------------------------------
   text = temp_text( parse );
   if ( ch == '\'' || ! ch ) {
      struct pos pos;
      t_init_pos( &pos,
         parse->source->file_entry_id,
         parse->source->line,
         parse->source->column );
      p_diag( parse, DIAG_POS_ERR, &pos,
         "missing character in character literal" );
      p_bail( parse );
   }
   if ( ch == '\\' ) {
      if ( parse->read_flags & READF_ESCAPESEQ ) {
         ch = read_ch( parse );
         if ( ch == '\'' ) {
            append_ch( text, ch );
            ch = read_ch( parse );
         }
         else {
            escape_ch( parse, &ch, text, false );
         }
      }
      else {
         append_ch( text, ch );
         ch = read_ch( parse );
         append_ch( text, ch );
         ch = read_ch( parse );
      }
   }
   else  {
      append_ch( text, ch );
      ch = read_ch( parse );
   }
   if ( ch != '\'' ) {
      struct pos pos;
      t_init_pos( &pos,
         parse->source->file_entry_id,
         parse->source->line,
         column );
      p_diag( parse, DIAG_POS_ERR, &pos,
         "multiple characters in character literal" );
      p_bail( parse );
   }
   ch = read_ch( parse );
   tk = TK_LIT_CHAR;
   goto finish;

   comment:
   // -----------------------------------------------------------------------
   while ( ch && ch != '\n' ) {
      ch = read_ch( parse );
   }
   goto whitespace;

   multiline_comment:
   // -----------------------------------------------------------------------
   while ( true ) {
      if ( ! ch ) {
         struct pos pos;
         t_init_pos( &pos, parse->source->file_entry_id, line, column );
         p_diag( parse, DIAG_POS_ERR, &pos,
            "unterminated comment" );
         p_bail( parse );
      }
      else if ( ch == '*' ) {
         ch = read_ch( parse );
         if ( ch == '/' ) {
            ch = read_ch( parse );
            goto whitespace;
         }
      }
      else {
         ch = read_ch( parse );
      }
   }

   finish:
   // -----------------------------------------------------------------------
   token->type = tk;
   if ( text != NULL ) {
      token->modifiable_text = t_intern_text( parse->task, text->value,
         text->length );
      token->text = token->modifiable_text;
      token->length = text->length;
   }
   else {
      const struct token_info* info = p_get_token_info( tk );
      token->modifiable_text = NULL;
      token->text = info->shared_text;
      token->length = ( length > 0 ) ?
         length : info->length;
   }
   token->pos.line = line;
   token->pos.column = column;
   token->pos.id = parse->source->file_entry_id;
   token->next = NULL;
}

static char read_ch( struct parse* parse ) {
   struct source* source = parse->source;
   // Adjust the file position. The file position is adjusted based on the
   // previous character. (If we adjust the file position based on the new
   // character, the file position will refer to the next character.)
   if ( source->ch == '\n' ) {
      ++source->line;
      source->column = 0;
      ++parse->line;
   }
   else if ( source->ch == '\t' ) {
      source->column += parse->task->options->tab_size -
         ( ( source->column + parse->task->options->tab_size ) %
         parse->task->options->tab_size );
   }
   else {
      ++source->column;
   }
   // Read character.
   enum { LOOKAHEAD_AMOUNT = 3 };
   enum { SAFE_AMOUNT = SOURCE_BUFFER_SIZE - LOOKAHEAD_AMOUNT };
   if ( source->buffer_pos >= SAFE_AMOUNT ) {
      size_t unread = SOURCE_BUFFER_SIZE - source->buffer_pos;
      memcpy( source->buffer, source->buffer + source->buffer_pos, unread );
      size_t count = fread( source->buffer + unread,
         sizeof( source->buffer[ 0 ] ), SOURCE_BUFFER_SIZE - unread,
         source->fh );
      if ( count != SOURCE_BUFFER_SIZE - unread &&
         ferror( source->fh ) != 0 ) {
         p_diag( parse, DIAG_ERR,
            "failed to read file: %s (%s)",
            parse->source->file->full_path.value, strerror( errno ) );
         p_bail( parse );
      }
      // Every line must be terminated by a newline character. If the end of
      // the file is not a newline character, implicitly generate one. For
      // empty files, this is not needed.
      if ( count < SOURCE_BUFFER_SIZE - unread && unread + count > 0 &&
         source->buffer[ unread + count - 1 ] != '\n' ) {
         source->buffer[ unread + count ] = '\n';
         source->buffer[ unread + count + 1 ] = '\0';
      }
      else {
         source->buffer[ unread + count ] = '\0';
      }
      source->buffer_pos = 0;
   }
   // Line concatenation.
   while ( source->buffer[ source->buffer_pos ] == '\\' ) {
      // Linux newline character.
      if ( source->buffer[ source->buffer_pos + 1 ] == '\n' ) {
         source->buffer_pos += 2;
         ++source->line;
         source->column = 0;
         ++parse->line;
      }
      // Windows newline character.
     else if ( source->buffer[ source->buffer_pos + 1 ] == '\r' &&
         source->buffer[ source->buffer_pos + 2 ] == '\n' ) {
         source->buffer_pos += 3;
         ++source->line;
         source->column = 0;
         ++parse->line;
      }
      else {
         break;
      }
   }
   // Process character.
   char ch = source->buffer[ source->buffer_pos ];
   if ( ch == '\r' && source->buffer[ source->buffer_pos + 1 ] == '\n' ) {
      // Replace the two-character Windows newline with a single-character
      // newline to simplify things.
      ch = '\n';
      source->buffer_pos += 2;
   }
   else {
      ++source->buffer_pos;
   }
   source->ch = ch;
   return ch;
}

static char peek_ch( struct parse* parse ) {
   return parse->source->buffer[ parse->source->buffer_pos ];
}

static void read_initial_ch( struct parse* parse ) {
   read_ch( parse );
   // The file position is adjusted based on the previous character. Initially,
   // there is no previous character, but the file position is still adjusted
   // when we read a character from read_ch(). We want the initial character to
   // retain the initial file position, so reset the file position.
   reset_filepos( parse->source );
}

static void escape_ch( struct parse* parse, char* ch_out, struct str* text,
   bool in_string ) {
   char ch = *ch_out;
   if ( ! ch ) {
      empty: ;
      struct pos pos = { parse->source->line, parse->source->column,
         parse->source->file_entry_id };
      p_diag( parse, DIAG_POS_ERR, &pos, "empty escape sequence" );
      p_bail( parse );
   }
   int slash = parse->source->column - 1;
   static const char singles[] = {
      'a', '\a',
      'b', '\b',
      'f', '\f',
      'n', '\n',
      'r', '\r',
      't', '\t',
      'v', '\v',
      0
   };
   int i = 0;
   while ( singles[ i ] ) {
      if ( singles[ i ] == ch ) {
         append_ch( text, singles[ i + 1 ] );
         ch = read_ch( parse );
         goto finish;
      }
      i += 2;
   }
   // Octal notation.
   char buffer[ 4 ];
   int code = 0;
   i = 0;
   while ( ch >= '0' && ch <= '7' ) {
      if ( i == 3 ) {
         too_many_digits: ;
         struct pos pos = { parse->source->line, parse->source->column,
            parse->source->file_entry_id };
         p_diag( parse, DIAG_POS_ERR, &pos, "too many digits" );
         p_bail( parse );
      }
      buffer[ i ] = ch;
      ch = read_ch( parse );
      ++i;
   }
   if ( i ) {
      buffer[ i ] = 0;
      code = strtol( buffer, NULL, 8 );
      goto save_ch;
   }
   if ( ch == '\\' ) {
      // In a string context, like the NUL character, the backslash character
      // must not be escaped.
      if ( in_string ) {
         append_ch( text, '\\' );
         append_ch( text, '\\' );
      }
      else {
         append_ch( text, '\\' );
      }
      ch = read_ch( parse );
   }
   // Hexadecimal notation.
   else if ( ch == 'x' || ch == 'X' ) {
      ch = read_ch( parse );
      i = 0;
      while (
         ( ch >= '0' && ch <= '9' ) ||
         ( ch >= 'a' && ch <= 'f' ) ||
         ( ch >= 'A' && ch <= 'F' ) ) {
         if ( i == 2 ) {
            goto too_many_digits;
         }
         buffer[ i ] = ch;
         ch = read_ch( parse );
         ++i;
      }
      if ( ! i ) {
         goto empty;
      }
      buffer[ i ] = 0;
      code = strtol( buffer, NULL, 16 );
      goto save_ch;
   }
   else {
      // In a string context, when encountering an unknown escape sequence,
      // leave it for the engine to process.
      if ( in_string ) {
         // TODO: Merge this code and the code above. Both handle the newline
         // character.
         if ( ch == '\n' ) {
            p_bail( parse );
         }
         append_ch( text, '\\' );
         append_ch( text, ch );
         ch = read_ch( parse );
      }
      else {
         struct pos pos = { parse->source->line, slash,
            parse->source->file_entry_id };
         p_diag( parse, DIAG_POS_ERR, &pos, "unknown escape sequence" );
         p_bail( parse );
      }
   }
   goto finish;

   save_ch:
   // -----------------------------------------------------------------------
   // Code needs to be a valid character.
   if ( code > 127 ) {
      struct pos pos = { parse->source->line, slash,
         parse->source->file_entry_id };
      p_diag( parse, DIAG_POS_ERR, &pos, "invalid character `\\%s`", buffer );
      p_bail( parse );
   }
   // In a string context, the NUL character must not be escaped. Leave it
   // for the engine to process it.
   if ( code == 0 && in_string ) {
      append_ch( text, '\\' );
      append_ch( text, '0' );
   }
   else {
      append_ch( text, ( char ) code );
   }

   finish:
   // -----------------------------------------------------------------------
   *ch_out = ch;
}

static struct str* temp_text( struct parse* parse ) {
   str_clear( &parse->temp_text );
   return &parse->temp_text;
}

static void append_ch( struct str* str, char ch ) {
   char segment[ 2 ] = { ch, '\0' };
   str_append( str, segment );
}

#if CHAR_MIN == 0

static void append_string_ch( struct str* text, char ch ) {
   append_ch( text, ( ch >= ' ' ) ? ch : ' ' );
}

#else

static void append_string_ch( struct str* text, char ch ) {
   append_ch( text, ( ! ( ch >= 0 && ch < ' ' ) ) ? ch : ' ' );
}

#endif

void p_increment_pos( struct pos* pos, enum tk tk ) {
   switch ( tk ) {
   case TK_BRACE_R:
      ++pos->column;
      break;
   default:
      break;
   }
}

void p_deinit_tk( struct parse* parse ) {
   if ( parse->source_entry ) {
      while ( parse->source_entry->source ) {
         p_pop_source( parse );
      }
   }
}
