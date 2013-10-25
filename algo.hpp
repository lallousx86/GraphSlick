#ifndef __ALGO__
#define __ALGO__

/*--------------------------------------------------------------------------
GraphSlick (c) Elias Bachaalany
-------------------------------------

Algorithms module

This module implements various algorithms used by the plugin

History
--------

10/23/2013 - eliasb             - First version, it comes from refactored code from the plugin module
10/24/2013 - eliasb             - Renamed class to a function like name and dropped the () operator
--------------------------------------------------------------------------*/


//--------------------------------------------------------------------------
#include <map>
#include <pro.h>
#include <funcs.hpp>
#include <gdl.hpp>
#include <graph.hpp>
#include "groupman.h"
#include "util.h"

//--------------------------------------------------------------------------
/**
* @brief Creates a mutable graph that have the combined nodes per the groupmanager
*        A class was used to simulate nested functions needed by the combine algo
*/
class fc_to_combined_mg
{
  // Create a mapping between single node ids and the nodedef list they belong to
  typedef std::map<nodedef_list_t *, int> ndl2id_t;
  ndl2id_t ndl2id;

  gnodemap_t *node_map;
  groupman_t *gm;
  qflow_chart_t *fc;
  bool show_nids_only;

  /**
  * @brief Create and return a groupped node ID
  */
  int get_ndlid(int n)
  {
    int ndl_id;

    // Find how this single node is defined in the group manager
    nodeloc_t *loc = gm->find_nodeid_loc(n);
    
    // Does this node have a group yet? (ndl)
    ndl2id_t::iterator it = ndl2id.find(loc->nl);
    if (it == ndl2id.end())
    {
      // Assign an auto-incr id
      ndl_id = ndl2id.size();  
      ndl2id[loc->nl] = ndl_id;

      // Initial text for this ndl is the current single node id
      gnode_t gn;
      gn.id = ndl_id;
      size_t t = loc->nl->size();
      for (nodedef_list_t::iterator it=loc->nl->begin();
           it != loc->nl->end();
           ++it)
      {
        if (show_nids_only)
        {
          gn.text.cat_sprnt("%d", it->nid);
          if (--t > 0)
            gn.text.append(", ");
        }
        else
        {
          qbasic_block_t &block = fc->blocks[it->nid];
          qstring s;
          get_disasm_text(
            block.startEA, 
            block.endEA, 
            &s);
          gn.text.append(s);
        }
      }

      // Cache the node data
      (*node_map)[ndl_id] = gn;
    }
    else
    {
      // Grab the ndl id
      ndl_id = it->second;
    }

    return ndl_id;
  }

  /**
  * @brief 
  */
  void build(
    qflow_chart_t &fc,
    groupman_t *gm,
    gnodemap_t &node_map,
    mutable_graph_t *mg)
  {
    // Take a reference to the local variables so they are used
    // in the other helper functions
    this->gm = gm;
    this->node_map = &node_map;
    this->fc = &fc;

    // Compute the total size of nodes needed for the combined graph
    // The size is the total count of node def lists in each group def
    size_t node_count = 0;
    for (groupdef_listp_t::iterator it=gm->get_groups()->begin();
         it != gm->get_groups()->end();
         ++it)
    {
      groupdef_t &gd = **it;
      node_count += gd.nodegroups.size();
    }

    // Resize the graph
    mg->resize(node_count);

    // Build the combined graph
    int snodes_count = fc.size();
    for (int n=0; n < snodes_count; n++)
    {
      // Figure out the combined node ID
      int ndl_id = get_ndlid(n);

      // Build the edges
      for (int isucc=0, succ_sz=fc.nsucc(n); isucc < succ_sz; isucc++)
      {
        // Get the successor node
        int nsucc = fc.succ(n, isucc);

        // This node belongs to the same NDL?
        int succ_ndlid = get_ndlid(nsucc);
        if (succ_ndlid == ndl_id)
        {
          // Do nothing, consider as one node
          continue;
        }
        // Add an edge
        mg->add_edge(ndl_id, succ_ndlid, NULL);
      }
    }
  }
public:
  /**
  * @brief Operator to call the class as a function
  */
  fc_to_combined_mg(
      ea_t ea,
      groupman_t *gm,
      gnodemap_t &node_map,
      mutable_graph_t *mg): show_nids_only(false)
  {
    // Build function's flowchart
    qflow_chart_t fc;
    if (!get_func_flowchart(ea, fc))
      return;

    build(fc, gm, node_map, mg);
  }
};

//--------------------------------------------------------------------------
bool func_to_mgraph(
    ea_t ea,
    mutable_graph_t *mg,
    gnodemap_t &node_map);

#endif