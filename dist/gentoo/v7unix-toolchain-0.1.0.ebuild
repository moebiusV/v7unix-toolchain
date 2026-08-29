# Copyright 2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

DESCRIPTION="V7 Unix PDP-11 C toolchain, modernized for a modern host"
HOMEPAGE="https://github.com/moebiusV/v7unix-toolchain"
SRC_URI="https://github.com/moebiusV/v7unix-toolchain/archive/refs/tags/v${PV}.tar.gz -> ${P}.tar.gz"
LICENSE="AncientUnix"
SLOT="0"
KEYWORDS="~amd64"

BDEPEND="sys-devel/bison dev-lang/python"
# recommended (optional): sys-fs/filsys, app-emulation/prebsd, app-emulation/simh

src_configure() {
	econf --libexecdir=/usr/libexec
}
