# Portable replacement for: xxd -i -n <VARNAME> <INPUT> > <OUTPUT>
# Usage: cmake -DINPUT=<path> -DOUTPUT=<path> -DVARNAME=<name> -P embed_resource.cmake

file(READ "${INPUT}" hex_data HEX)
string(LENGTH "${hex_data}" hex_len)
math(EXPR byte_count "${hex_len} / 2")

set(array_content "")
set(col 0)
set(line_buf "  ")
math(EXPR last_idx "${byte_count} - 1")

foreach(i RANGE ${last_idx})
  math(EXPR pos "${i} * 2")
  string(SUBSTRING "${hex_data}" ${pos} 2 byte)
  if(col GREATER 0)
    string(APPEND line_buf ", ")
  endif()
  string(APPEND line_buf "0x${byte}")
  math(EXPR col "${col} + 1")
  if(col EQUAL 12 OR i EQUAL last_idx)
    if(i LESS last_idx)
      string(APPEND line_buf ",")
    endif()
    string(APPEND array_content "${line_buf}\n")
    set(line_buf "  ")
    set(col 0)
  endif()
endforeach()

file(WRITE "${OUTPUT}" "unsigned char ${VARNAME}[] = {
${array_content}};
unsigned int ${VARNAME}_len = ${byte_count};
")
