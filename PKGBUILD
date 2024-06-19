# Maintainer: Wellington <wellingtonwallace@gmail.com>

pkgname=eyeofsauron-git
pkgver=eosqt.r0.g137e67c
pkgrel=1
pkgdesc='Optimize system performance for games'
arch=(x86_64 i686)
url='https://github.com/wwmm/eyeofsauron'
license=('GPL3')
depends=(
  'boost-libs' 
  'kirigami' 
  'kirigami-addons' 
  'qqc2-desktop-style' 
  'breeze-icons' 
  'qt6-base' 
  'qt6-multimedia' 
  'qt6-charts' 
  'opencv'
  'linux-api-headers')
makedepends=('boost' 'cmake' 'extra-cmake-modules' 'git' 'ninja' 'intltool' 'appstream-glib' 'libmediainfo' 'fftw')
#source=("git+https://github.com/wwmm/eyeofsauron.git#branch=eosqt")
source=("git+https://github.com/wwmm/eyeofsauron.git")
conflicts=(eyeofsauron)
provides=(eyeofsauron)
sha512sums=('SKIP')

pkgver() {
  cd eyeofsauron
  git describe --long | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
  #git describe --long --all | sed 's/^v//;s/^heads\///;s/\([^-]*-g\)/r\1/;s/-/./g'
}

build() {
  cmake \
    -B build  \
    -S eyeofsauron \
    -G Ninja \
    -DCMAKE_INSTALL_PREFIX:PATH='/usr' \
    -Wno-dev

  cmake --build build
}

package() {
  DESTDIR="${pkgdir}" cmake --install build
}
