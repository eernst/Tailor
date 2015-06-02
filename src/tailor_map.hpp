/*
# Tailor, a BWT-based aligner for non-templated RNA tailing
# Copyright (C) 2014 Min-Te Chou, Bo W Han, Jui-Hung Hung
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef TAILOR_MAP_HPP_
#define TAILOR_MAP_HPP_

#include <string>
#include <boost/program_options.hpp>
#include "tailer.hpp"

namespace tailor_map {

int main (int argc, char** argv) {
	std::string usage = R"(

*********************************************************************************
+------+
|Tailor|
+------+
    Tailor uses BWT to perform genomic mapping with ability to detect non-templated
addition of nucleotide to the 3' end of the query sequence (tailing).
    All hits will be aligned to a reference sequence with exact match. Any unmapped
sequences at the 3' end are considered "tail". The exact matching process is
equivalent to -v 0 -a mode of bowtie.
    Tailor also offer to allow mismatches in the middle of the query string. But
this is not the default behavior.
    Reports will be in SAM format. Tails will be described as "soft-clip" in CIGAR
and the sequences are reported under "TL:Z:" in the optional fields. Mismatches, if
allowed, will be reported in the "MD" tag.

    Tailor is freely avaible on github: jhhung.github.com/Tailor

# To map sequences in a fastq file to against an index.

>  tailor map


*********************************************************************************


)";
	/** option variables **/
	std::string inputFastq {};
	std::string indexPrefix {};
	std::string outputSAM {};
	bool allow_mm;
	std::size_t nthread {};
	int minLen {};
	boost::program_options::options_description opts {usage};
	try {
		opts.add_options ()
            ("help,h", "display this help message and exit")
            ("input,i", boost::program_options::value<std::string>(&inputFastq)->required(), "Input fastq file")
            ("index,p", boost::program_options::value<std::string>(&indexPrefix)->required(), "Prefix of the index")
            ("output,o", boost::program_options::value<std::string>(&outputSAM)->default_value(std::string{"stdout"}), "Output SAM file, stdout by default ")
            ("thread,n", boost::program_options::value<std::size_t>(&nthread)->default_value(1), "Number of thread to use; if the number is larger than the core available, it will be adjusted automatically")
            ("minLen,l", boost::program_options::value<int>(&minLen)->default_value(18), "minimal length of exact match (prefix match) allowed")
            ("mismatch,v", boost::program_options::bool_switch(&allow_mm)->default_value(false), "to allow mismatch in the middle of the query")
        ;
		boost::program_options::variables_map vm;
		boost::program_options::store (boost::program_options::parse_command_line(argc, argv, opts), vm);
		boost::program_options::notify(vm);
		if (vm.count("help") || argc < 4)	{ std::cerr << opts << std::endl; exit (1); }
	}
	catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << opts << std::endl;
		exit (1);
	} catch (...) {
		std::cerr << "Unknown error!" << std::endl;
		std::cerr << opts << std::endl;
		exit (1);
	}

	/** check index **/
	if (indexPrefix.back () != '.') {
		indexPrefix += '.';
	}
	if (!checkIndexIntact (indexPrefix)) {
		std::cerr << "Error: index files appear to be damaged. Please rebuild them.\nExiting..." << std::endl;
		exit (2);
	}
	/** check input fastq **/
	if (!boost::filesystem::exists (inputFastq)) {
		std::cerr << "Error: Input fastq file " << inputFastq << " does not exist! Please double check.\nExiting..." << std::endl;
		exit (1);
	}
	/** check output **/
	std::ostream* out{nullptr};
	if (outputSAM == "stdout" || outputSAM == "-") {
		out = &std::cout;
	} else {
		out = new std::ofstream {outputSAM};
		if (!*out) {
			std::cerr << "Error: cannot create output file " << outputSAM << ".\nPlease double check.\nExiting..." << std::endl;
			exit (1);
		}
	}
	/** check thread **/
	auto nCore = boost::thread::hardware_concurrency();
	if ( nCore != 0 && nthread > nCore) {
		std::cerr << "Warning: the number of threads set (" << nthread << ") is larger than the number of cores available (" << nCore << ") in this machine.\nSo reset -n=" << nCore << std::endl;
		nthread = nCore;
	}
	/** execute mapping **/
	tailing2 (indexPrefix, inputFastq, out, nthread, minLen, allow_mm);
	/** close file handle **/
	if (out != &std::cout) {
		static_cast<std::ofstream*>(out)-> close ();
		delete out;
	}
}

}

#endif
