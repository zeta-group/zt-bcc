project('bcc', 'c')

src = [
    'src/gbuf.c',
    'src/main.c',
    'src/cache/cache.c',
    'src/cache/archive.c',
    'src/cache/field.c',
    'src/cache/library.c',
    'src/common.c',
    'src/parse/dec.c',
    'src/parse/stmt.c',
    'src/parse/expr.c',
    'src/parse/asm.c',
    'src/parse/phase.c',
    'src/parse/token/queue.c',
    'src/parse/token/source.c',
    'src/parse/token/stream.c',
    'src/parse/token/info.c',
    'src/parse/token/expr.c',
    'src/parse/token/output.c',
    'src/parse/token/dirc.c',
    'src/parse/token/user.c',
    'src/parse/library.c',
    'src/version.c',
    'src/semantic/dec.c',
    'src/semantic/type.c',
    'src/semantic/stmt.c',
    'src/semantic/expr.c',
    'src/semantic/asm.c',
    'src/semantic/phase.c',
    'src/codegen/dec.c',
    'src/codegen/obj.c',
    'src/codegen/stmt.c',
    'src/codegen/expr.c',
    'src/codegen/asm.c',
    'src/codegen/pcode.c',
    'src/codegen/phase.c',
    'src/codegen/linear.c',
    'src/codegen/chunk.c',
    'src/task.c',
    'src/builtin.c'
]

tests = [
    'test/jm.bcs',
    'test/list.bcs',
    'test/parse_fixed.bcs',
    'test/bigint.bcs',
    'test/stack.bcs',
    'test/sorting.bcs',
    'test/functions.bcs'
]

dirs = [
    'src',
    'src/parse'
]

incdirs = include_directories(dirs)

bcc = executable('bcc', sources: src, include_directories: incdirs)


# Test BCS compilation
testscript = find_program('scripts/test.sh', required: false)

if testscript.found()
    foreach t : tests
        test('[BCS compilation] @0@'.format(t.split('/')[-1]), testscript, args: [files(t), '@0@/lib'.format(meson.current_source_dir())])
    endforeach

else
    warning('Test script (scripts/test.sh) not found, not generating tests.')
    
endif
