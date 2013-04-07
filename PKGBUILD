# Maintainer: Jesse McClure AKA "Trilby" <jmcclure [at] cns [dot] umass [dot] edu>
pkgname=ttwm-git
_gitname="ttwm"
pkgver=175
pkgrel=1
pkgdesc="Tiny Tiler/Tabbed Tiler Window Manager"
url="http://github.com/TrilbyWhite/ttwm.git"
arch=('any')
license=('GPLv3')
depends=('libx11' 'libxrandr')
makedepends=('git')
source=("$_gitname::git+git://github.com/TrilbyWhite/ttwm.git")
sha256sums=('SKIP')

pkgver() {
	cd $_gitname
	git rev-list HEAD --count
}

prepare() {
	cd $_gitname
	## Check XDG_CONFIG_HOME
	if [[ -f $XDG_CONFIG_HOME/ttwm/config.h ]]; then
		msg "Using user config from $XDG_CONFIG_HOME/config.h"
		msg "  Be sure to check for changes to default config.h"
		cp $XDG_CONFIG_HOME/ttwm/config.h ./config.h
	else
		msg "Using default config"
	fi
	if [[ -f $XDG_CONFIG_HOME/ttwm/icons.h ]]; then
		msg "Using user icons from $XDG_CONFIG_HOME/icons.h"
		cp $XDG_CONFIG_HOME/ttwm/icons.h ./icons.h
	else
		msg "Using default icons"
	fi
	## Check ~
	if [[ -f /.ttwm_conf.h ]]; then
		msg "Using user config from ~/.ttwm_conf.h"
		msg "  Be sure to check for changes to default config.h"
		cp ~/.ttwm_conf.h ./config.h
	else
		msg "Using default config"
	fi
	if [[ -f ~/.ttwm_icons.h ]]; then
		msg "Using user icons from ~/.ttwm_icons.h"
		cp ~/.ttwm_icons.h ./icons.h
	else
		msg "Using default icons"
	fi
}

build() {
	cd $_gitname
	make
}

package() {
	cd "$_gitname"
	make PREFIX=/usr DESTDIR=$pkgdir install
}

