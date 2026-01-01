# === ZION ---------------------------------------------------------------------
#
# Any client code should define ZION_PATH first and then include that file.
# LIBZION, the static archive library, will be defined and provided an action to
# make it.
#
# Example code
#
#     ZION_PATH = ../../zion/c
#     include ${ZION_PATH}/zion.mk
#
#     a.out: main.c ${LIBZION}
#         ${CC} -o $@ main.c ${CFLAGS} ${LDFLAGS} ${LIBZION}
#

LIBZION  = ${ZION_PATH}/.build/libzion.a

${LIBZION}:
	make -C ${ZION_PATH} release
