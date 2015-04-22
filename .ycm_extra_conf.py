import os
import ycm_core

flags = [
'-Wall',
'-std=c++11',
'-x',
'c++',
'-DYCM',
'-isystem/tarball/compiz-1.0.0/compiz-1.0.0/include',
'-isystem/usr/bin/../lib64/gcc/x86_64-unknown-linux-gnu/4.9.1/../../../../include/c++/4.9.1',
'-isystem/usr/bin/../lib64/gcc/x86_64-unknown-linux-gnu/4.9.1/../../../../include/c++/4.9.1/x86_64-unknown-linux-gnu',
'-isystem/usr/bin/../lib64/gcc/x86_64-unknown-linux-gnu/4.9.1/../../../../include/c++/4.9.1/backward',
'-isystem/usr/local/include',
'-isystem/usr/bin/../lib/clang/3.4.2/include/',
'-isystem/usr/include',
'-DUSE_CLANG_COMPLETER',
]

compilation_database_folder = ''

if compilation_database_folder:
  database = ycm_core.CompilationDatabase( compilation_database_folder )
else:
  database = None


def DirectoryOfThisScript():
  return os.path.dirname( os.path.abspath( __file__ ) )


def MakeRelativePathsInFlagsAbsolute( flags, working_directory ):
  if not working_directory:
    return list( flags )
  new_flags = []
  make_next_absolute = False
  path_flags = [ '-isystem', '-I', '-iquote', '--sysroot=' ]
  for flag in flags:
    new_flag = flag

    if make_next_absolute:
      make_next_absolute = False
      if not flag.startswith( '/' ):
        new_flag = os.path.join( working_directory, flag )

    for path_flag in path_flags:
      if flag == path_flag:
        make_next_absolute = True
        break

      if flag.startswith( path_flag ):
        path = flag[ len( path_flag ): ]
        new_flag = path_flag + os.path.join( working_directory, path )
        break

    if new_flag:
      new_flags.append( new_flag )
  return new_flags


def FlagsForFile( filename ):
  if database:
    compilation_info = database.GetCompilationInfoForFile( filename )
    final_flags = MakeRelativePathsInFlagsAbsolute(
      compilation_info.compiler_flags_,
      compilation_info.compiler_working_dir_ )

    try:
      final_flags.remove( '-stdlib=libc++' )
    except ValueError:
      pass
  else:
    relative_to = DirectoryOfThisScript()
    final_flags = MakeRelativePathsInFlagsAbsolute( flags, relative_to )

  return {
    'flags': final_flags,
    'do_cache': True
  }
