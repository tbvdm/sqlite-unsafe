PROG=		sqlite-unsafe
NOMAN=

PKGS?=		sqlite3

.if !(make(clean) || make(cleandir) || make(obj))
CFLAGS+!=	pkg-config --cflags ${PKGS}
LDFLAGS+!=	pkg-config --libs ${PKGS}
.endif

.include <bsd.prog.mk>
