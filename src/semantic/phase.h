#ifndef SRC_SEMANTIC_PHASE_H
#define SRC_SEMANTIC_PHASE_H

#include "../task.h"

struct follower {
   struct path* path;
   union {
      struct object* object;
      struct enumeration* enumeration;
      struct structure* structure;
      struct ns* ns;
   } result;
   int requested_node;
};

struct object_search {
   const char* name;
   struct pos* pos;
   struct object* object;
   int requested_node;
};

struct type_info {
   struct ref* ref;
   struct structure* structure;
   struct enumeration* enumeration;
   struct dim* dim;
   int spec;
   int storage;
   union {
      struct ref ref;
      struct ref_struct structure;
      struct ref_array array;
      struct ref_func func;
   } implicit_ref;
   bool builtin_func;
};

struct func_test {
   struct func* func;
   struct list* labels;
   struct list* funcscope_vars;
   struct func* nested_funcs;
   struct return_stmt* returns;
   struct func_test* parent;
   struct script* script;
   struct buildmsg* enclosing_buildmsg;
};

struct stmt_test {
   struct stmt_test* parent;
   struct switch_stmt* switch_stmt;
   struct buildmsg* buildmsg;
   struct jump* jump_break;
   struct jump* jump_continue;
   struct type_info cond_type;
   enum {
      FLOW_GOING,
      FLOW_BREAKING,
      FLOW_DEAD,
      FLOW_RESET,
   } flow;
   bool in_loop;
   bool manual_scope;
   bool case_allowed;
   bool found_folded_case;
};

struct expr_test {
   jmp_buf bail;
   struct type_info type;
   struct buildmsg* buildmsg;
   struct name* name_offset;
   struct var* var;
   struct structure_member* structure_member;
   struct func* func;
   struct indexed_string_usage* magic_id_usage;
   struct dim* dim;
   int dim_depth;
   bool result_required;
   bool has_str;
   bool undef_erred;
   bool suggest_paren_assign;
};

struct type_snapshot {
   struct ref* ref;
   struct structure* structure;
   struct enumeration* enumeration;
   struct dim* dim;
   int spec;
   int storage;
};

struct type_iter {
   struct type_info key;
   struct type_info value;
   bool available;
};

enum subscript_result {
   SUBSCRIPTRESULT_SUBARRAY,
   SUBSCRIPTRESULT_REF,
   SUBSCRIPTRESULT_STRUCT,
   SUBSCRIPTRESULT_PRIMITIVE
};

enum type_description {
   TYPEDESC_NONE,
   TYPEDESC_ARRAY,
   TYPEDESC_ARRAYREF,
   TYPEDESC_STRUCTREF,
   TYPEDESC_FUNCREF,
   TYPEDESC_NULLREF,
   TYPEDESC_STRUCT,
   TYPEDESC_ENUM,
   TYPEDESC_PRIMITIVE
};

enum deprecation {
   // Description: using `.` operator on namespaces. Recommend using `::`
   // instead.
   DEPRECATION_NSDOT,
   // Description: using the Length() function of arrays and strings. Recommend
   // using lengthof() for arrays and StrLen() for strings.
   DEPRECATION_ASSOCFUNCLENGTH,
   DEPRECATION_TOTAL
};

struct semantic {
   struct task* task;
   struct library* main_lib;
   struct library* lib;
   struct ns* ns;
   struct ns_fragment* ns_fragment;
   struct scope* scope;
   struct scope* func_scope;
   struct scope* free_scope;
   struct sweep* free_sweep;
   struct func_test* topfunc_test;
   struct func_test* func_test;
   const struct lang_limits* lang_limits;
   struct var* world_vars[ MAX_WORLD_VARS ];
   struct var* world_arrays[ MAX_WORLD_VARS ];
   struct var* global_vars[ MAX_GLOBAL_VARS ];
   struct var* global_arrays[ MAX_GLOBAL_VARS ];
   struct type_info type_int;
   int depth;
   bool retest_nss;
   bool resolved_objects;
   bool trigger_err;
   bool in_localscope;
   bool strong_type;
   struct {
      bool registered;
      bool suppressed;
   } deprecations[ DEPRECATION_TOTAL ];
};

void s_init( struct semantic* semantic, struct task* task );
void s_test( struct semantic* semantic );
void s_test_constant( struct semantic* semantic, struct constant* );
void s_test_enumeration( struct semantic* semantic, struct enumeration* );
void s_test_struct( struct semantic* semantic, struct structure* structure );
void s_test_var( struct semantic* semantic, struct var* var );
void s_test_func( struct semantic* semantic, struct func* func );
void s_test_func_body( struct semantic* semantic, struct func* func );
void s_test_local_var( struct semantic* semantic, struct var* );
void s_test_foreach_var( struct semantic* semantic,
   struct type_info* collection_type, struct var* var );
void s_init_expr_test( struct expr_test* test, bool result_required,
   bool suggest_paren_assign );
void s_init_expr_test_enumerator( struct expr_test* test,
   struct enumeration* enumeration );
void s_test_expr( struct semantic* semantic, struct expr_test*, struct expr* );
void s_test_bool_expr( struct semantic* semantic, struct expr* expr );
void s_init_stmt_test( struct stmt_test*, struct stmt_test* );
void s_test_top_block( struct semantic* semantic, struct block* block );
void s_test_func_block( struct semantic* semantic, struct func* func,
   struct block* block );
void s_add_scope( struct semantic* semantic, bool func_scope );
void s_pop_scope( struct semantic* semantic );
void s_test_script( struct semantic* semantic, struct script* script );
void s_calc_var_size( struct var* var );
void s_calc_var_value_index( struct var* var );
void s_bind_name( struct semantic* semantic, struct name* name,
   struct object* object );
void s_bind_local_name( struct semantic* semantic, struct name* name,
   struct object* object, bool block_scope );
void s_diag( struct semantic* semantic, int flags, ... );
void s_bail( struct semantic* semantic );
void p_test_inline_asm( struct semantic* semantic, struct stmt_test* test,
   struct inline_asm* inline_asm );
void s_init_type_info( struct type_info* type, struct ref* ref,
   struct structure* structure, struct enumeration* enumeration,
   struct dim* dim, int spec, int storage );
void s_init_type_info_copy( struct type_info* type,
   struct type_info* other_type );
void s_init_type_info_array_ref( struct type_info* type, struct ref* ref,
   struct structure* structure, struct enumeration* enumeration,
   int dim_count, int spec );
void s_init_type_info_func( struct type_info* type, struct ref* ref,
   struct structure* structure, struct enumeration* enumeration,
   struct param* params, int return_spec, int min_param, int max_param,
   bool local );
void s_init_type_info_builtin_func( struct type_info* type );
void s_init_type_info_scalar( struct type_info* type, int spec );
void s_init_type_info_null( struct type_info* type );
bool s_same_type( struct type_info* a, struct type_info* b );
bool s_instance_of( struct type_info* type, struct type_info* instance );
void s_present_type( struct type_info* type, struct str* string );
bool s_is_ref_type( struct type_info* type );
bool s_is_value_type( struct type_info* type );
bool s_is_primitive_type( struct type_info* type );
void s_iterate_type( struct semantic* semantic, struct type_info* type,
   struct type_iter* iter );
void s_init_object_search( struct object_search* search, int requested_node,
   struct pos* pos, const char* name );
void s_search_object( struct semantic* semantic,
   struct object_search* search );
void s_init_follower( struct follower* follower, struct path* path,
   int requested_node );
void s_follow_path( struct semantic* semantic, struct follower* follower );
struct path* s_last_path_part( struct path* path );
void s_perform_using( struct semantic* semantic, struct using_dirc* dirc );
void s_unknown_ns_object( struct semantic* semantic, struct ns* ns,
   const char* object_name, struct pos* pos );
bool s_is_scalar( struct type_info* type );
bool s_is_str_value_type( struct type_info* type );
void s_take_type_snapshot( struct type_info* type,
   struct type_snapshot* snapshot );
void s_take_fine_type_snapshot( struct type_info* type,
   struct type_snapshot* snapshot, bool force_dup_ref );
bool s_is_onedim_int_array( struct type_info* type );
bool s_is_int_value( struct type_info* type );
bool s_is_str_value( struct type_info* type );
struct alias* s_alloc_alias( void );
void s_init_alias( struct alias* alias );
void s_test_type_alias( struct semantic* semantic, struct type_alias* alias );
void s_decay( struct semantic* semantic, struct type_info* type );
void s_type_mismatch( struct semantic* semantic, const char* label_a,
   struct type_info* type_a, const char* label_b, struct type_info* type_b,
   struct pos* pos );
void s_test_nested_func( struct semantic* semantic, struct func* func );
int s_spec( struct semantic* semantic, int spec );
struct object* s_get_ns_object( struct ns* ns, const char* object_name,
   int requested_node );
struct object* s_get_direct_ns_object( struct ns* ns, const char* object_name,
   int requested_node );
struct object* s_get_local_object( struct semantic* semantic,
   const char* object_name, int requested_node );
struct object* s_get_object( struct semantic* semantic,
   const char* object_name, int requested_node );
bool s_is_enumerator( struct type_info* type );
bool s_is_null( struct type_info* type );
bool s_is_nullable( struct type_info* type );
bool s_in_msgbuild_block( struct semantic* semantic );
bool s_is_array_ref( struct type_info* type );
bool s_is_struct( struct type_info* type );
bool s_is_onedim_int_array_ref( struct semantic* semantic,
   struct type_info* type );
enum subscript_result s_subscript_array_ref( struct semantic* semantic,
   struct type_info* type, struct type_info* element_type );
enum type_description s_describe_type( struct type_info* type );
bool s_is_ref( struct type_info* type );
bool s_common_type( struct type_info* a, struct type_info* b,
   struct type_info* result );
bool s_is_void( struct type_info* type );
void s_reveal( struct type_info* type );
bool s_is_str( struct type_info* type );
bool s_is_struct_ref( struct type_info* type );
bool s_same_storageignored_type( struct type_info* a, struct type_info* b );
void s_init_magic_id( struct magic_id* magic_id, int name );
bool s_deprecation( struct semantic* semantic, enum deprecation deprecation );
void s_register_deprecation( struct semantic* semantic,
   enum deprecation deprecation );

#endif
