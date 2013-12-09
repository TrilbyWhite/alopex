# Maintainer: Jesse AKA "Trilby" <jesse [at] mcclurek9 [dot] com>
_gitname="alopex"
pkgname="${_gitname}-git"
pkgver=3.291.1763f16
pkgrel=1
pkgdesc='A Tabbed Tiling Window Manager with Fur'
#url='http://trilbywhite.github.io/alopex/'
url='http://github.com/TrilbyWhite/alopex.git'
arch=('any')
license=('GPLv3')
depends=('libx11' 'libxrandr' 'cairo' 'freetype2')
makedepends=('git' 'imagemagick')
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
	make DESTDIR="${pkgdir}" install
}
