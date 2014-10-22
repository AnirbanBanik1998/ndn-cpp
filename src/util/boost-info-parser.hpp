/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2014 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * From PyNDN boost_info_parser by Adeola Bannis.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version, with the additional exemption that
 * compiling, linking, and/or using OpenSSL is allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * A copy of the GNU Lesser General Public License is in the file COPYING.
 */

#ifndef NDN_BOOST_INFO_PARSER_HPP
#define NDN_BOOST_INFO_PARSER_HPP

#include <string>
#include <vector>
#include <utility>
#include <ndn-cpp/common.hpp>

namespace ndn {

/**
 * BoostInfoTree is provided for compatibility with the Boost INFO property list
 * format used in ndn-cxx.
 *
 * Each node in the tree may have a name and a value as well as associated
 * sub-trees. The sub-tree names are not unique, and so sub-trees are stored as
 * dictionaries where the key is a sub-tree name and the values are the
 * sub-trees sharing the same name.
 *
 * Nodes can be accessed with a path syntax, as long as nodes in the path do not
 * contain the path separator '/' in their names.
 */
class BoostInfoTree
{
public:
  BoostInfoTree(const std::string& value = "", BoostInfoTree* parent = 0)
  : value_(value), parent_(parent), lastChild_(0)
  {
  }

  void
  createSubtree(const std::string& treeName, const std::string& value = "");

  /**
   * Use treeName to find the vector of BoostInfoTree in subTrees_. Even though
   * this returns a copy of the vector of BoostInfoTree, this method is not
   * const because it is a shallow copy and the caller can still modify the
   * BoostInfoTree elements.
   * @param key The key in subTrees_ to search for.
   * @return A new shallow copy of the vector of BoostInfoTree.
   */
  std::vector<ptr_lib::shared_ptr<BoostInfoTree> >
  operator [] (const std::string& key)
  {
    return *find(key);
  }

  const std::string& getValue() const { return value_; }

  BoostInfoTree*
  getParent() { return parent_; }

  BoostInfoTree*
  getLastChild() { return lastChild_; }

  std::string
  prettyPrint(int indentLevel = 1) const;

private:
  /**
   * Use treeName to find the vector of BoostInfoTree in subTrees_.
   * @param value The key in subTrees_ to search for.
   * @return A pointer to the vector of BoostInfoTree, or 0 if not found.
   */
  std::vector<ptr_lib::shared_ptr<BoostInfoTree> >*
  find(const std::string& treeName);

  // We can't use a map for subTrees_ because we want the keys to be in order.
  std::vector<std::pair<std::string, std::vector<ptr_lib::shared_ptr<BoostInfoTree> > > > subTrees_;
  std::string value_;
  BoostInfoTree* parent_;
  BoostInfoTree* lastChild_;
};

inline std::ostream&
operator << (std::ostream& os, const BoostInfoTree& tree)
{
  os << tree.prettyPrint();
  return os;
}

/**
 * A BoostInfoParser reads files in Boost's INFO format and constructs a
 * BoostInfoTree.
 */
class BoostInfoParser
{
public:
  BoostInfoParser()
  : root_(new BoostInfoTree())
  {
  }

  /**
   * Add the contents of the file to the root BoostInfoTree.
   * @param fileName The path to the INFO file.
   * @return The new root BoostInfoTree.
   */
  const BoostInfoTree&
  read(const std::string& fileName);

  /**
   * Write the root tree of this BoostInfoParser as file in Boost's INFO format.
   * @param fileName The output path.
   */
  void
  write(const std::string& fileName) const;

  /**
   * Get the root tree of this parser.
   * @return The root BoostInfoTree.
   */
  const BoostInfoTree&
  getRoot() const { return *root_; }

private:
  /**
   * Similar to Python's shlex.split, split s into an array of strings which are
   * separated by whitespace, treating a string within quotes as a single entity
   * regardless of whitespace between the quotes. Also allow a backslash to
   * escape the next character.
   * @param s The input string to split.
   * @param result This appends the split strings to result. This does not first
   * clear the vector.
   */
  static void
  shlex_split(const std::string& s, std::vector<std::string>& result);

  BoostInfoTree*
  parseLine(const std::string& line, BoostInfoTree* context);

  BoostInfoTree* root_;
};

}

#endif
