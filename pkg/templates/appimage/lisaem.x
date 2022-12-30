echo this is just a test, don't use yet
exit 0

source /etc/os-release
cat > "${TLD}/pkg/tmp/appimage.yml" <<END
app: lisaem
ingredients:
  dist: $VERSION_CODENAME
  sources:
    - ${TLD}
  debs:
    - ${TLD}/pkg/.... TODO ....
END

bash -ex pkg2appimage "${TLD}/pkg/tmp/appimage.yml"

