from conan import ConanFile
from conan.tools.cmake import CMakeToolchain
from conan.tools.cmake import cmake_layout

#required_conan_version = ">=1.55.0" # also in conan.conf

class ReSIProcateConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps"
    name = "resiprocate"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_cares": [True, False],
        "with_fmt": [True, False],
        "with_kurento": [True, False],
        "with_mysql": [True, False],
        "with_postgresql": [True, False],
        "with_ssl": [True, False],
        "with_repro": [True, False],
        "with_recon": [True, False],
        "with_reconserver": [True, False],
        "with_return": [True, False],
        "with_tfm": [True, False],
    }
    default_options = {
        "fPIC": True,
        "shared": False,
        "with_cares": True,
        "with_fmt": True,
        "with_kurento": True,
        "with_mysql": True,
        "with_postgresql": True,
        "with_ssl": True,
        "with_repro": True,
        "with_recon": True,
        "with_reconserver": True,
        "with_return": True,
        "with_tfm": False,
    }

    #def package_id(self):
    #    self.info.requires.package_revision_mode()

    #def build_requirements(self):
    #    self.tool_requires("benchmark/1.7.1")

    def requirements(self):
        if self.options.with_cares:
            self.requires("c-ares/1.19.0")
        if self.options.with_fmt:
            self.requires("fmt/9.1.0")
        if self.options.with_kurento:
            self.requires("websocketpp/0.8.2")
        if self.options.with_repro:
            #self.requires("libdb/5.3.28")
            self.requires("libdb/5.3.28@gjasny/testing") # https://github.com/conan-io/conan-center-index/pull/16385
            self.requires("pcre/8.45")
            self.requires("cajun-jsonapi/2.1.1")
            if self.options.with_mysql:
                self.requires("libmysqlclient/8.0.31")
            if self.options.with_postgresql:
                self.requires("libpq/14.5")
        if self.options.with_recon:
            self.requires("libsrtp/2.4.2")
            if self.options.with_mysql or self.options.with_postgresql:
                self.requires("soci/4.0.3")
        if self.options.with_reconserver:
            if self.options.with_mysql or self.options.with_postgresql:
                self.requires("soci/4.0.3")
        if self.options.with_return or self.options.with_kurento:
            self.requires("asio/1.24.0")
        if self.options.with_ssl:
            self.requires("openssl/1.1.1t") # want 3.0.8
        if self.options.with_tfm:
            self.requires("cppunit/1.15.1")

    def configure(self):
        self.options['libdb'].with_cxx = True
        self.options['soci'].with_mysql = self.options.with_mysql
        self.options['soci'].with_postgresql = self.options.with_postgresql
        self.options['websocketpp'].asio = 'standalone'

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        tc.cache_variables["USE_CONAN"] = True
        tc.cache_variables["USE_CONTRIB"] = False
        tc.cache_variables["BUILD_QPID_PROTON"] = False
        tc.cache_variables["REGENERATE_MEDIA_SAMPLES"] = False

        tc.cache_variables["WITH_C_ARES"] = self.options.with_cares
        tc.cache_variables["USE_FMT"] = self.options.with_fmt
        tc.cache_variables["USE_KURENTO"] = self.options.with_kurento
        tc.cache_variables["WITH_SSL"] = self.options.with_ssl
        tc.cache_variables["USE_MYSQL"] = self.options.with_repro and self.options.with_mysql
        tc.cache_variables["USE_POSTGRESQL"] = self.options.with_repro and self.options.with_postgresql
        tc.cache_variables["USE_SOCI_MYSQL"] = self.options.with_reconserver and self.options.with_mysql
        tc.cache_variables["USE_SOCI_POSTGRESQL"] = self.options.with_reconserver and self.options.with_postgresql
        tc.cache_variables["BUILD_REPRO"] = self.options.with_repro
        tc.cache_variables["BUILD_RECON"] = self.options.with_recon
        tc.cache_variables["BUILD_RECONSERVER"] = self.options.with_reconserver
        tc.cache_variables["BUILD_RETURN"] = self.options.with_return
        tc.cache_variables["BUILD_TFM"] = self.options.with_tfm
        #if self.options.with_ssl:
        #if self.options.with_repro:
        #if self.options.with_recon:
        #tc.cache_variables["CMAKE_C_COMPILER"] = '/usr/bin/clang-14'
        tc.generate()

    def layout(self):
        cmake_layout(self, build_folder="_build")
