// ==========================================================================
//                 SeqAn - The Library for Sequence Analysis
// ==========================================================================
//
// Copyright (c) 2006-2018, Knut Reinert, FU Berlin
// Copyright (c) 2016-2018, Knut Reinert & MPI Molekulare Genetik
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Knut Reinert or the FU Berlin nor the names of
//       its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL KNUT REINERT OR THE FU BERLIN BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// ==========================================================================

#include <gtest/gtest.h>

#include <seqan3/alignment/aligned_sequence/aligned_sequence_concept.hpp>
#include <seqan3/alphabet/all.hpp>
#include <seqan3/io/alignment_file/detail.hpp>
#include <seqan3/io/stream/debug_stream.hpp>
#include <seqan3/range/view/persist.hpp>
#include <seqan3/range/view/convert.hpp>

using namespace seqan3;

template <typename T>
class aligned_sequence_test: public ::testing::Test { };

using range_types = ::testing::Types<std::vector<gapped<dna4>>,
                                     std::vector<qualified<gapped<dna4>, phred42>>,
                                     std::vector<gapped<qualified<dna4, phred42>>>>;

// Initializer function is needed for the typed test because the gapped_decorator
// will be initialized differently than the naive vector<gapped<dna>>.
template <sequence_container_concept container_type>
void initialize_typed_test_container(container_type & container, std::string const && target)
{
    for (auto & val : target)
    {
        typename container_type::value_type cval{};
        assign_char(cval, val);
        container.push_back(cval);
    }
}

TYPED_TEST_CASE(aligned_sequence_test, range_types);

TYPED_TEST(aligned_sequence_test, aligned_sequence_concept)
{
    EXPECT_TRUE((aligned_sequence_concept<TypeParam>));
}

TYPED_TEST(aligned_sequence_test, insert_one_gap)
{
    TypeParam aligned_seq;
    TypeParam aligned_seq_expected;
    initialize_typed_test_container(aligned_seq, "ACTA");
    initialize_typed_test_container(aligned_seq_expected, "A-CTA");

    auto it = insert_gap(aligned_seq, aligned_seq.begin() + 1);

    EXPECT_EQ(*it, gap::GAP);
    EXPECT_EQ(aligned_seq, aligned_seq_expected);
}

TYPED_TEST(aligned_sequence_test, insert_multiple_gaps)
{
    TypeParam aligned_seq;
    TypeParam aligned_seq_expected;
    initialize_typed_test_container(aligned_seq, "ACTA");
    initialize_typed_test_container(aligned_seq_expected, "A--CTA");

    auto it = insert_gap(aligned_seq, aligned_seq.begin() + 1, 2);

    EXPECT_EQ(*it, gap::GAP);
    EXPECT_EQ(*++it, gap::GAP);
    EXPECT_EQ(aligned_seq, aligned_seq_expected);
}

TYPED_TEST(aligned_sequence_test, erase_one_gap)
{
    // 1) Removing an actual gap
    TypeParam aligned_seq;
    TypeParam aligned_seq_expected;
    initialize_typed_test_container(aligned_seq, "A-CTA");
    initialize_typed_test_container(aligned_seq_expected, "ACTA");

    auto it = erase_gap(aligned_seq, aligned_seq.begin() + 1);

    typename TypeParam::value_type val{'C'_dna4};
    val = 'C'_dna4;
    EXPECT_EQ(*it, val);
    EXPECT_EQ(aligned_seq, aligned_seq_expected);

    // 2) Removing a non-gap
    TypeParam aligned_seq_fail;
    initialize_typed_test_container(aligned_seq_fail, "A-CTA");

    EXPECT_THROW(erase_gap(aligned_seq_fail, aligned_seq_fail.begin() + 2), gap_erase_failure);

    EXPECT_EQ(aligned_seq_fail[1], gap::GAP); // nothing has changed
}

TYPED_TEST(aligned_sequence_test, erase_multiple_gaps)
{
    // 1) Removing actual gaps

    // nucleotide alphabet
    TypeParam aligned_seq;
    TypeParam aligned_seq_expected;
    initialize_typed_test_container(aligned_seq, "A--CTA");
    initialize_typed_test_container(aligned_seq_expected, "ACTA");

    auto it = erase_gap(aligned_seq, aligned_seq.begin() + 1, aligned_seq.begin() + 3);

    typename TypeParam::value_type val{'C'_dna4};
    val = 'C'_dna4;
    EXPECT_EQ(*it, val);
    EXPECT_EQ(aligned_seq, aligned_seq_expected);

    // 2) Removing a non-gap
    TypeParam aligned_seq_fail;
    initialize_typed_test_container(aligned_seq_fail, "A-CTA");

    EXPECT_THROW(erase_gap(aligned_seq_fail, aligned_seq_fail.begin() + 1, aligned_seq_fail.begin() + 3),
                 gap_erase_failure);

    EXPECT_EQ(aligned_seq_fail[1], gap::GAP); // nothing has changed
}

TYPED_TEST(aligned_sequence_test, cigar_string)
{
    {    // default_parameters
        TypeParam ref;
        TypeParam read;
        initialize_typed_test_container(ref,  "ACGTGAT--CTG");
        initialize_typed_test_container(read, "ACGT-CGTAGTG");

//         [[maybe_unused]] char c = detail::compare_aligned_values(ref[0], read[0], false);

        std::string expected = "4M1D2M2I3M";
//         std::string two = detail::get_cigar_string(std::tie(ref, read));

        EXPECT_EQ(expected, detail::get_cigar_string(std::make_pair(ref, read)));

        TypeParam ref2;
        TypeParam read2;
        initialize_typed_test_container(ref2,  "---ACGTGAT--CTG--");
        initialize_typed_test_container(read2, "-ACGT-CGTAGTG----");

        std::string expected2 = "1P2I2M1D4M2I1M2D2P";

        EXPECT_EQ(expected2, detail::get_cigar_string(std::make_pair(ref2, read2)));
    }
    {
        // with soft clipping
        TypeParam ref;
        TypeParam read;
        initialize_typed_test_container(ref,  "ACGTGAT--CTG");
        initialize_typed_test_container(read, "ACGT-CGTAGTG");

        std::string expected = "5S4M1D2M2I3M60S";

        EXPECT_EQ(expected, detail::get_cigar_string(std::make_pair(ref, read), 5, 60));

        // gaps at the end
        TypeParam ref2;
        TypeParam read2;
        initialize_typed_test_container(ref2,  "---ACGTGAT--CTG--");
        initialize_typed_test_container(read2, "-ACGT-CGTAGTG----");

        std::string expected2 = "3S1P2I2M1D4M2I1M2D2P5S";

        EXPECT_EQ(expected2, detail::get_cigar_string(std::make_pair(ref2, read2), 3, 5));
    }
    {
        // no gaps at the end
        TypeParam ref;
        TypeParam read;
        initialize_typed_test_container(ref,  "ACGTGAT--CAG");
        initialize_typed_test_container(read, "ACGT-CGTACTG");

        std::string expected1 =    "4=1D2X2I1=1X1=";
        std::string expected2 = "5S4=1D2X2I1=1X1=60S";

        EXPECT_EQ(expected1, detail::get_cigar_string(std::make_pair(ref, read), 0, 0, true));
        EXPECT_EQ(expected2, detail::get_cigar_string(std::make_pair(ref, read), 5, 60, true));
    }
}

TEST(aligned_sequence_stream, tuple)
{
    std::string const expected
    {
        "      0     .    :    .    :    .    :    .    :    .    :\n"
        "        GCGGGTCACTGAGGGCTGGGATGAGGACGGCCACCACTTCGAGGAGTCCC\n"
        "            | ||      |        |  |       |   |||   |    |\n"
        "        CTACGGCAGAAGAAGACATCCGAAAAAGCTGACACCTCTCGCCTACAAGC\n"
        "        ||||||||||||||||||||| || |||||||||||||||||||||||||\n"
        "        CTACGGCAGAAGAAGACATCCCAAGAAGCTGACACCTCTCGCCTACAAGC\n"
        "\n"
        "     50     .    :    .    :    .    :    .    :    .    :\n"
        "        TTCACTACGAGGGCAGGGCCGTGGACATCACCACGTCAGACAGGGACAAG\n"
        "            |            || | | | | |     | |   | |     | \n"
        "        AGTTCATACCTAATGTCGCGGAGAAGACCTTAGGGGCCAGCGGCAGATAC\n"
        "        |||| |||||||||||||||||||||||||||||||||||||||||||||\n"
        "        AGTTTATACCTAATGTCGCGGAGAAGACCTTAGGGGCCAGCGGCAGATAC\n"
        "\n"
        "    100     .    :    .    :    .    :    .    :\n"
        "        AGCAAGTACGGCACCCTGTCCAGACTGGCGGTGGAAGCTG\n"
        "               |    || |          |    |  |||   \n"
        "        GAGGGCAAGATAACGCGCAATTCGGAGAGATTTAAAGAAC\n"
        "        ||||||||||| ||||||||||||||||||||||||||||\n"
        "        GAGGGCAAGATCACGCGCAATTCGGAGAGATTTAAAGAAC\n"
    };

    std::tuple<std::vector<gapped<dna4>>, std::vector<gapped<dna4>>, std::vector<gapped<dna4>>> const alignment
    {
        "GCGGGTCACTGAGGGCTGGGATGAGGACGGCCACCACTTCGAGGAGTCCCTTCACTACGAGGGCAGGGCCGTGGACATCACCACGTCAGACAGGGACAAGAGCAAGTA"
        "CGGCACCCTGTCCAGACTGGCGGTGGAAGCTG"_dna4 | view::persist | view::convert<gapped<dna4>>,
        "CTACGGCAGAAGAAGACATCCGAAAAAGCTGACACCTCTCGCCTACAAGCAGTTCATACCTAATGTCGCGGAGAAGACCTTAGGGGCCAGCGGCAGATACGAGGGCAA"
        "GATAACGCGCAATTCGGAGAGATTTAAAGAAC"_dna4 | view::persist | view::convert<gapped<dna4>>,
        "CTACGGCAGAAGAAGACATCCCAAGAAGCTGACACCTCTCGCCTACAAGCAGTTTATACCTAATGTCGCGGAGAAGACCTTAGGGGCCAGCGGCAGATACGAGGGCAA"
        "GATCACGCGCAATTCGGAGAGATTTAAAGAAC"_dna4 | view::persist | view::convert<gapped<dna4>>
    };

    std::ostringstream oss;
    debug_stream_type stream{oss};
    stream << alignment;
    EXPECT_EQ(expected, oss.str());
}

TEST(aligned_sequence_stream, pair_of_different_types)
{
    std::string const expected
    {
        "      0     . \n"
        "        CUUC-G\n"
        "        ||   |\n"
        "        CU-NGG\n"
    };

    std::pair<std::vector<gapped<rna4>>, std::vector<gapped<rna5>>> const alignment
    {
        {'C'_rna4, 'U'_rna4, 'U'_rna4, 'C'_rna4, gap::GAP, 'G'_rna4},
        {'C'_rna5, 'U'_rna5, gap::GAP, 'N'_rna5, 'G'_rna5, 'G'_rna5}
    };

    std::ostringstream oss;
    debug_stream_type stream{oss};
    stream << alignment;
    EXPECT_EQ(expected, oss.str());
}
