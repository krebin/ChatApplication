PROTOPATHS =
for(p, PROTOPATH):PROTOPATHS += --proto_path=$${p}

grpc_decl.name = grpc headers
grpc_decl.input = PROTOS
grpc_decl.output = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.grpc.pb.h
grpc_decl.commands = protoc --grpc_out=${QMAKE_FILE_IN_PATH} --plugin=protoc-gen-grpc=$(EXPORT_GRPC_CPP_PLUGIN_PATH) --proto_path=${QMAKE_FILE_IN_PATH} ${QMAKE_FILE_NAME}
grpc_decl.variable_out = HEADERS
QMAKE_EXTRA_COMPILERS += grpc_decl

grpc_impl.name = grpc sources
grpc_impl.input = PROTOS
grpc_impl.output = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.grpc.pb.cc
grpc_impl.depends = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.grpc.pb.h
grpc_impl.commands = $$escape_expand(\n)
grpc_impl.variable_out = SOURCES
QMAKE_EXTRA_COMPILERS += grpc_impl
