# Maintainer: Fabien Pollet <mail@frmpollet.me>

pkgname=wmpulsemixer
pkgver=0.2.2
pkgrel=1
pkgdesc="A simple PulseAudio mixer for WindowMaker"
url="https://github.com/polletfa/wmpulsemixer"
license=("GPL-2.0-only")
depends=('libxext' 'libxpm' 'libx11' 'libpulse' 'gtk3')
options+=('!debug')
arch=('i686' 'x86_64' 'aarch64')
source=("${pkgname}-${pkgver}.tar.gz::https://github.com/polletfa/wmpulsemixer/archive/refs/tags/${pkgver}.tar.gz")
sha256sums=('caf6f2181302b85875e86cfeba281c6af600bc5bbf2ec3b259230be4a3bdfa20')

build() {
	cd "$srcdir/$pkgname-$pkgver"
	make
}

package() {
	cd "$srcdir/$pkgname-$pkgver"
	make install DESTDIR="$pkgdir" PREFIX=/usr
}
