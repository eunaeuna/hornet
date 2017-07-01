/**
 * @author Federico Busato                                                  <br>
 *         Univerity of Verona, Dept. of Computer Science                   <br>
 *         federico.busato@univr.it
 * @date June, 2017
 * @version v1.3
 *
 * @copyright Copyright © 2017 cuStinger. All rights reserved.
 *
 * @license{<blockquote>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * </blockquote>}
 */
#include "GraphIO/GraphBase.hpp"
#include "Support/Host/Basic.hpp"   //WARNING
#include "Support/Host/FileUtil.hpp"//xlib::file_size
#include "Support/Host/Numeric.hpp" //xlib::check_overflow
#include <iostream>                 //std::cout
#include <sstream>                  //std::istringstream

namespace graph {

ParsingProp::ParsingProp(const detail::ParsingEnum& value) noexcept :
                 xlib::PropertyClass<detail::ParsingEnum, ParsingProp>(value) {}

bool ParsingProp::is_sort() const noexcept {
    return *this & parsing_prop::SORT;
}

bool ParsingProp::is_randomize() const noexcept {
    return *this & parsing_prop::RANDOMIZE;
}

bool ParsingProp::is_print() const noexcept {
    return *this & parsing_prop::PRINT;
}
//------------------------------------------------------------------------------

StructureProp::StructureProp(const detail::StructureEnum& value) noexcept :
             xlib::PropertyClass<detail::StructureEnum, StructureProp>(value) {}

bool StructureProp::is_directed() const noexcept {
    return *this & structure_prop::DIRECTED;
}

bool StructureProp::is_undirected() const noexcept {
    return *this & structure_prop::UNDIRECTED;
}

bool StructureProp::is_reverse() const noexcept {
    return *this & structure_prop::REVERSE;
}

bool StructureProp::is_coo() const noexcept {
    return *this & structure_prop::COO;
}

bool StructureProp::is_direction_set() const noexcept {
    return is_directed() || is_undirected();
}

/*
inline bool StructureProp::is_weighted() const noexcept {
    using namespace detail;
    return _wtype != NONE;
}*/
//==============================================================================

template<typename vid_t, typename eoff_t>
inline GraphBase<vid_t, eoff_t>::
GraphBase(vid_t nV, eoff_t nE, const StructureProp& structure) noexcept :
        _nV(nV), _nE(nE), _structure(structure) {}

template<typename vid_t, typename eoff_t>
inline GraphBase<vid_t, eoff_t>::GraphBase(StructureProp structure) noexcept :
                                                    _structure(structure) {}

template<typename vid_t, typename eoff_t>
inline const std::string& GraphBase<vid_t, eoff_t>::name() const noexcept {
    return _graph_name;
}

template<typename vid_t, typename eoff_t>
inline void GraphBase<vid_t, eoff_t>
::set_structure(const StructureProp& structure) noexcept {
    _structure = structure;
}

//==============================================================================

template<typename vid_t, typename eoff_t>
void GraphBase<vid_t, eoff_t>::read(const char* filename,
                                    const ParsingProp& prop) {
    xlib::check_regular_file(filename);
    size_t size = xlib::file_size(filename);
    _graph_name = xlib::extract_filename(filename);
    _prop       = prop;

    if (prop.is_print()) {
        std::cout << "\nGraph File:\t" << _graph_name
                  << "       Size: " <<  xlib::format(size / xlib::MB) << " MB";
    }

    std::string file_ext = xlib::extract_file_extension(filename);
    if (file_ext == ".bin") {
        if (prop.is_print())
            std::cout << "            (Binary)Reading...   \n";
        if (prop.is_print() && (prop.is_randomize() || prop.is_sort()))
            std::cerr << "#input sort/randomize ignored on binary format\n";
        readBinary(filename, prop.is_print());
        return;
    }

    std::ifstream fin;
    //IO improvements START ----------------------------------------------------
    const int BUFFER_SIZE = 1 * xlib::MB;
    char buffer[BUFFER_SIZE];
    //std::ios_base::sync_with_stdio(false);
    fin.tie(nullptr);
    fin.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);
    //IO improvements END ------------------------------------------------------
    fin.open(filename);
    std::string first_str;
    fin >> first_str;
    fin.seekg(std::ios::beg);

    if (file_ext == ".mtx" && first_str == "%%MatrixMarket") {
        if (prop.is_print())
            std::cout << "            (Market)\n";
        readMarket(fin, prop.is_print());
    }
    else if (file_ext == ".graph") {
        if (prop.is_print())
            std::cout << "        (Dimacs10th)\n";
        if (prop.is_randomize() || prop.is_sort()) {
            std::cerr << "#input sort/randomize ignored on Dimacs10th format"
                      << std::endl;
        }
        readDimacs10(fin, prop.is_print());
    }
    else if (file_ext == ".gr" && (first_str == "c"|| first_str == "p")) {
        if (prop.is_print())
            std::cout << "         (Dimacs9th)\n";
        readDimacs9(fin, prop.is_print());
    }
    else if (file_ext == ".txt" && first_str == "#") {
        if (prop.is_print())
            std::cout << "              (SNAP)\n";
        readSnap(fin, prop.is_print());
    }
    else if (file_ext == ".edges") {
        if (prop.is_print())
            std::cout << "    (Net Repository)\n";
        readNetRepo(fin, prop.is_print());
    }
    else if (first_str == "%") {
        if (prop.is_print())
            std::cout << "            (Konect)\n";
        readKonect(fin, prop.is_print());
    } else
        ERROR("Graph type not recognized");
    fin.close();
    COOtoCSR();
}

//==============================================================================

template<typename vid_t, typename eoff_t>
GInfo GraphBase<vid_t, eoff_t>::getMarketHeader(std::ifstream& fin) {
    std::string header_lines;
    std::getline(fin, header_lines);
    auto direction = header_lines.find("symmetric") != std::string::npos ?
                        structure_prop::UNDIRECTED : structure_prop::DIRECTED;
    _directed_to_undirected = direction == structure_prop::UNDIRECTED;

    /*if (header_lines.find("integer") != std::string::npos)
        _structure._wtype = Structure::INTEGER;
    if (header_lines.find("real") != std::string::npos)
        _structure._wtype = Structure::REAL;*/

    while (fin.peek() == '%')
        xlib::skip_lines(fin);

    size_t rows, columns, num_lines;
    fin >> rows >> columns >> num_lines;
    if (rows != columns)
        WARNING("Rectangular matrix");
    xlib::skip_lines(fin);
    size_t num_edges = direction == structure_prop::UNDIRECTED ? num_lines * 2
                                                               : num_lines;
    return { std::max(rows, columns), num_edges, num_lines, direction };
}

//------------------------------------------------------------------------------

template<typename vid_t, typename eoff_t>
GInfo GraphBase<vid_t, eoff_t>::getDimacs9Header(std::ifstream& fin) {
    while (fin.peek() == 'c')
        xlib::skip_lines(fin);

    xlib::skip_words(fin, 2);
    size_t num_vertices, num_edges;
    fin >> num_vertices >> num_edges;
    return { num_vertices, num_edges, num_edges, structure_prop::UNDIRECTED };
}

//------------------------------------------------------------------------------

template<typename vid_t, typename eoff_t>
GInfo GraphBase<vid_t, eoff_t>::getDimacs10Header(std::ifstream& fin) {
    while (fin.peek() == '%')
        xlib::skip_lines(fin);

    size_t num_vertices, num_edges;
    fin >> num_vertices >> num_edges;
    StructureProp direction;

    if (fin.peek() == '\n')
        direction = structure_prop::UNDIRECTED;
    else {
        std::string flag;
        fin >> flag;
        direction = flag == "100" ? structure_prop::DIRECTED
                                  : structure_prop::UNDIRECTED;
        xlib::skip_lines(fin);
    }
    _directed_to_undirected = direction == structure_prop::UNDIRECTED;
    if (direction == structure_prop::UNDIRECTED)
        num_edges *= 2;
    return { num_vertices, num_edges, num_vertices, direction };
}

//------------------------------------------------------------------------------

template<typename vid_t, typename eoff_t>
GInfo GraphBase<vid_t, eoff_t>::getKonectHeader(std::ifstream& fin) {
    std::string str;
    fin >> str >> str;
    auto direction = (str == "asym") || (str == "bip") ?
                        structure_prop::DIRECTED : structure_prop::UNDIRECTED;
    size_t num_edges, value1, value2;
    fin >> str >> num_edges >> value1 >> value2;
    xlib::skip_lines(fin);
    if (str != "%")
        ERROR("Wrong file format")
    _directed_to_undirected = direction == structure_prop::UNDIRECTED;
    return { std::max(value1, value2), num_edges, num_edges, direction };
}

//------------------------------------------------------------------------------

template<typename vid_t, typename eoff_t>
void GraphBase<vid_t, eoff_t>::getNetRepoHeader(std::ifstream& fin) {
    std::string str;
    fin >> str >> str;
    //_header_direction = (str == "directed") ? Structure::DIRECTED
    //                                        : Structure::UNDIRECTED;
    xlib::skip_lines(fin);
}

//------------------------------------------------------------------------------

template<typename vid_t, typename eoff_t>
GInfo GraphBase<vid_t, eoff_t>::getSnapHeader(std::ifstream& fin) {
    std::string tmp;
    fin >> tmp >> tmp;
    StructureProp direction = (tmp == "Undirected") ? structure_prop::UNDIRECTED
                                                    : structure_prop::DIRECTED;
    xlib::skip_lines(fin);

    size_t num_lines = 0, num_vertices = 0;
    while (fin.peek() == '#') {
        std::getline(fin, tmp);
        if (tmp.substr(2, 6) == "Nodes:") {
            std::istringstream stream(tmp);
            stream >> tmp >> tmp >> num_vertices >> tmp >> num_lines;
            break;
        }
    }
    xlib::skip_lines(fin);
    _directed_to_undirected = direction == structure_prop::UNDIRECTED;
    return { num_vertices, num_lines, num_lines, direction };
}

//------------------------------------------------------------------------------

template class GraphBase<int, int>;
template class GraphBase<int64_t, int64_t>;

} // namespace graph
