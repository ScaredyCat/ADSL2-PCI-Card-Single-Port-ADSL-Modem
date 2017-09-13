#! /bin/sh

bindir=$PWD

target="`dirname ${bindir}`"

# Work out how not do an echo without a final line-feed
if test "`/bin/echo 'foo\c'`" = 'foo\c'; then
    ECHON="/bin/echo -n"
    ECHOE=""
else
    ECHON="/bin/echo"
    ECHOE='\c'
fi

cat <<EOF                                       >$target/bin/toolchain-env.sh
PATH=${target}/bin:${target}/usr/bin:\$PATH
export PATH

MANPATH="\${MANPATH}:${target}/man"
export MANPATH

CCACHE_PATH=${target}/bin-ccache:\$CCACHE_PATH
export CCACHE_PATH

EOF

cat <<EOF                                       >$target/bin/toolchain-env.csh
set path = (${target}/bin \$path)

if ( \$?MANPATH ) then
  setenv MANPATH "\${MANPATH}:${target}/man"
else
  setenv MANPATH ":${target}/man"
endif

if ( \$?CCACE_PATH ) then
  setenv CCACE_PATH "\${CCACE_PATH}:${target}/bin-ccache"
else
  setenv CCACE_PATH ":${target}/bin-ccache"
endif

EOF

chmod a+x $target/bin/toolchain-env.sh
chmod a+x $target/bin/toolchain-env.csh

if test -r ${target}/usr/bin/fakeroot; then
    FAKEROOT_BASE_DIR=${target}
    sed -i "s,^PREFIX=.*,PREFIX=$FAKEROOT_BASE_DIR/usr,g" $FAKEROOT_BASE_DIR/usr/bin/fakeroot
    sed -i "s,^BINDIR=.*,BINDIR=$FAKEROOT_BASE_DIR/usr/bin,g" $FAKEROOT_BASE_DIR/usr/bin/fakeroot
fi

echo ""
echo "Setup of tool chain is complete"
echo ""

