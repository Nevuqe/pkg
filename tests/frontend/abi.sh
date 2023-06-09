#! /usr/bin/env atf-sh

. $(atf_get_srcdir)/test_environment.sh
tests_init \
	basic

basic_body() {
	_uname_s="$(uname -s)"
	_expected="TODO: implement me"
		# Otherwise the ABI should be parsed from the ELF file.
		_expected="FreeBSD:13:amd64\n"
	atf_check \
		-o inline:"${_expected}" \
		pkg -o IGNORE_OSMAJOR=1 -o ABI_FILE=$(atf_get_srcdir)/fbsd.bin config abi

#	atf_check \
#		-o inline:"dragonfly:5.10:x86:64\n" \
#		pkg -o IGNORE_OSMAJOR=1 -o ABI_FILE=$(atf_get_srcdir)/dfly.bin config abi
}
