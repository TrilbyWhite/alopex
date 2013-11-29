# Maintainer: Jesse McClure AKA "Trilby" <jmcclure [at] cns [dot] umass [dot] edu>
_gitname="alopex"
pkgname="${_gitname}-git"
pkgver=3.291.1763f16
pkgrel=1
pkgdesc="A Tiny, Tabbed, Tiling Window Manager with Fur"
url="http://trilbywhite.github.io/alopex/"
arch=('any')
license=('GPLv3')
depends=('libx11' 'libxrandr')
makedepends=('git')
source=("${_gitname}::git://github.com/TrilbyWhite/alopex.git")
sha256sums=('SKIP')

pkgver() {
	cd "${_gitname}";
	echo "3.$(git rev-list --count HEAD).$(git describe --always )"
}

build() {
	cd "${_gitname}"
	make
}

package() {
	cd "${_gitname}"
	make PREFIX=/usr DESTDIR="${pkgdir}" install
}
