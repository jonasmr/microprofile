# SPDX-FileCopyrightText: 2021 Andrea Pappacoda <andrea@pappacoda.it>
#
# SPDX-License-Identifier: MIT

project(
	'microprofile',
	'cpp',
	default_options: [
		'warning_level=3',
		'b_lto=true',
		'b_ndebug=if-release',
		'cpp_std=c++11'
	],
	license: 'MIT',
	meson_version: '>=0.46.0'
)

# Dependencies

deps = [dependency('threads')]

extra_args = []

compiler = meson.get_compiler('cpp')

stb_dep = dependency('stb', required: false)
if stb_dep.found()
	deps += stb_dep
	extra_args += '-DMICROPROFILE_SYSTEM_STB'
endif

if host_machine.system() == 'windows'
	deps += compiler.find_library('ws2_32')
endif

# Check if -latomic is needed - https://github.com/pistacheio/pistache/blob/8545c0ca5a65fcc2415bf3f886e9f9f0699fd4ff/meson.build#L45
compiler_id = compiler.get_id()

cxx_atomics_check_code = '''
#include <atomic>
std::atomic<int> x;
std::atomic<short> y;
std::atomic<char> z;
int main() {
	++z;
	++y;
	return ++x;
}
'''
has_working_cxx_atomics = compiler.links(cxx_atomics_check_code, name: 'std::atomic')
if (compiler_id == 'clang' or compiler_id == 'gcc') and not has_working_cxx_atomics
	libatomic_dep = compiler.find_library('atomic')
	has_working_cxx_atomics = compiler.links(cxx_atomics_check_code, dependencies: libatomic_dep, name: 'std::atomic with libatomic')
	assert(has_working_cxx_atomics, 'Host compiler must support std::atomic')
	deps += libatomic_dep
endif

cxx_atomics64_check_code = '''
#include <atomic>
#include <cstdint>
std::atomic<uint64_t> x (0);
int main() {
	uint64_t i = x.load(std::memory_order_relaxed);
	(void)i;
	return 0;
}
'''
has_working_cxx_atomics64 = compiler.links(cxx_atomics64_check_code, name: 'std::atomic<uint64_t>')
if (compiler_id == 'clang' or compiler_id == 'gcc') and not has_working_cxx_atomics64
	libatomic_dep = compiler.find_library('atomic')
	has_working_cxx_atomics = compiler.links(cxx_atomics64_check_code, dependencies: libatomic_dep, name: 'std::atomic<uint64_t> with libatomic')
	assert(has_working_cxx_atomics, 'Host compiler must support 64-bit std::atomic')
	deps += libatomic_dep
endif

include_dir = include_directories('.')

# Custom config

if get_option('microprofile_use_config')
	extra_args += '-DMICROPROFILE_USE_CONFIG'
	install_headers('microprofile.config.h')
endif

# Main build target

libmicroprofile = library(
	'microprofile',
	'microprofile.cpp',
	cpp_args: extra_args,
	dependencies: deps,
	include_directories: include_dir,
	install: true
)

# Headers and pkg-config installation

install_headers(
	'microprofile.h',
	'microprofile_html.h'
)

import('pkgconfig').generate(
	libmicroprofile,
	description: 'microprofile is an embeddable profiler',
	extra_cflags: extra_args,
	url: 'https://github.com/jonasmr/microprofile'
)

# Declaring the dependency so that microprofile can be used as a subproject

microprofile_dep = declare_dependency(
	compile_args: extra_args,
	include_directories: include_dir,
	link_with: libmicroprofile
)

if meson.version().version_compare('>=0.54.0')
	meson.override_dependency('microprofile', microprofile_dep)
endif
