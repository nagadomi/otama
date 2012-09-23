require 'mkmf'

dir_config('otama')
dir_config('ruby')
if (have_header('otama.h') && have_library('otama', 'otama_open'))
  create_makefile 'otama'
else
  warn "error: Can't locate libotama."
  exit(-1)
end


