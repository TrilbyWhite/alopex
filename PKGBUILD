# Maintainer: Jesse McClure AKA "Trilby" <jmcclure [at] cns [dot] umass [dot] edu>
_gitname="alopex"
pkgname="${_gitname}-git"
pkgver=0.181.1a63f31
pkgrel=1
pkgdesc="A Tiny, Tabbed, Tiling Window Manager with Fur"
url="http://github.com/TrilbyWhite/${pkgname//-/.}"
arch=('any')
license=('GPLv3')
depends=('libx11' 'libxrandr')
makedepends=('git')
source=("${_gitname}::git://github.com/TrilbyWhite/${pkgname//-/.}")
sha256sums=('SKIP')

pkgver() {
	cd "${_gitname}";
	echo 1.$(git rev-list --count HEAD).$(git describe --always )
}

prepare() {
	for config in {"$HOME/.${_gitname}_","$XDG_CONFIG_HOME/${_gitname}"/}{conf,config,icons}.h; do
		if [[ -f "$config" ]]; then
			case "$config" in
				*conf.h | *config.h)
					cp "$config" "${srcdir}/${_gitname}/config.h"
					echo "Using configuration from $config"
					echo "Check the default config.h for changes"
					;;
				*icons.h)
					cp "$config" "${srcdir}/${_gitname}/icons.h"
					echo "Using icons from $config"
					;;
			esac
		fi
	done
}

build() {
	cd "${_gitname}"
	make
	## try different initial themes:
	##   WinterCoat is assumed if none are specified.
	#MOLT=WinterCoat make
	#MOLT=SummerCoat make
}

package() {
	cd "${_gitname}"
	make PREFIX=/usr DESTDIR="${pkgdir}" install
}
