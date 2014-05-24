# Modify these paths to your needs and then build the project

# Add here the paths that are not in the global PATH or INCLUDEPATH
# environment variables usually for mobile OS
android|!linux: {
    ZEROMQ_INCLUDE_PATH = /home/mah/android/android-libs/include
    ZEROMQ_LIB_PATH = /home/mah/android/android-libs/lib
    ZEROMQ_LIB_FLAGS = -Bstatic
    PROTOBUF_INCLUDE_PATH = /home/mah/android/android-libs/include
    PROTOBUF_LIB_PATH = /home/mah/android/android-libs/lib
    PROTOBUF_LIB_FLAGS = -Bstatic
    PROTOBUF_PROTOC = /usr/local/bin/protoc
}
ios: {
    ZEROMQ_INCLUDE_PATH = /opt/zeromq-ios/include
    ZEROMQ_LIB_PATH = /opt/zeromq-ios/lib
    ZEROMQ_LIB_FLAGS = -Bstatic
    PROTOBUF_INCLUDE_PATH = /opt/protobuf-ios/include
    PROTOBUF_LIB_PATH = /opt/protobuf-ios/lib
    PROTOBUF_LIB_FLAGS = -Bstatic
    PROTOBUF_PROTOC = /opt/protobuf-ios/bin/protoc
    LIBSODIUM_LIB_PATH = /opt/libsodium-ios/lib
}
macx: {
    ZEROMQ_INCLUDE_PATH = /opt/local/include
    ZEROMQ_LIB_PATH = /opt/local/lib
    PROTOBUF_INCLUDE_PATH = /opt/local/include
    PROTOBUF_LIB_PATH = /opt/local/lib
}

linux|!android: {
    ZEROMQ_INCLUDE_PATH = /usr/local/include
    ZEROMQ_LIB_PATH = /usr/local/lib
    ZEROMQ_LIB_FLAGS = -Bstatic
    PROTOBUF_INCLUDE_PATH = /usr/local/include
    PROTOBUF_LIB_PATH = /usr/local/lib
    PROTOBUF_LIB_FLAGS = -Bstatic
    PROTOBUF_PROTOC = /usr/local/bin/protoc
#    LIBSODIUM_LIB_PATH = /opt/lib
}


# Qt SDK directory - autodetected based on qmake path
QTSDK_DIR=$$absolute_path($$[QT_INSTALL_PREFIX]/../..)

# Qt Creator paths for the Qt Quick Designer plugin
# Path to the Qt Creator source - not needed anymore
# QTCREATOR_SOURCE_DIR=/home/alexander/bin/qt-creator
# Path to installed Qt Creator (where the plugin should be installed to)
QTCREATOR_INSTALL_DIR=$$QTSDK_DIR/Tools/QtCreator

# Qt documentation directory containing Qt documentation with .index files
# -> somehow only the Android toolchain comes with .index files
# install one of the Android toolchains to work properly
QT_DOC_DIR = $$[QT_INSTALL_DOCS]
