set path = (/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/bin $path)

if ( $?MANPATH ) then
  setenv MANPATH "${MANPATH}:/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/man"
else
  setenv MANPATH ":/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/man"
endif

if ( $?CCACE_PATH ) then
  setenv CCACE_PATH "${CCACE_PATH}:/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/bin-ccache"
else
  setenv CCACE_PATH ":/opt/uclibc-toolchain/ifx-lxdb-1-1/gcc-3.3.6/toolchain-mips/bin-ccache"
endif

