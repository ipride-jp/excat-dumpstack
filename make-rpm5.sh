DS_=DumpStack_forJava5
DSV=${DS_}-1.0.0
TMP=`mktemp -d`

pushd ${TMP}
svn co https://www.ipride.co.jp/svn/error_analyzer/trunk/${DS_}/
mv ${DS_} ${DSV}
pushd ${DSV}
find . -name .svn -exec rm -fr \{\} \;
rm -f DumpStack.???
rm -fr Debug/ excat_win/ include/ lib/ libDumpStack.so.1.0
popd
rm -f ${DSV}.tar ${DSV}.tar.gz
tar -cvf ${DSV}.tar ${DSV}
gzip -v9 ${DSV}.tar
cp -f ${DSV}.tar.gz /usr/src/redhat/SOURCES/
cp -f ${DSV}/${DS_}.spec /usr/src/redhat/SPECS/
pushd /usr/src/redhat/SPECS/
rpmbuild -ba ${DS_}.spec
popd
rm -f ${DSV}.tar.gz
rm -fr ${DSV}
popd
cp -f /usr/src/redhat/SRPMS/${DS_}*.rpm .
cp -f /usr/src/redhat/RPMS/i386/${DS_}*.rpm .
rm -fr ${TMP}

# the end of file
