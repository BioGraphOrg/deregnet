// --------------------------------------------------------------------------
//                   deregnet -- find deregulated pathways
// --------------------------------------------------------------------------
// Copyright Sebastian Winkler --- Eberhard Karls University Tuebingen, 2016
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Sebastian Winkler $
// $Authors: Sebastian Winkler $
// --------------------------------------------------------------------------
//

#include <set>

#include <deregnet/usinglemon.h>
#include <deregnet/StartHeuristic.h>

using namespace std;

namespace deregnet {

StartHeuristic::StartHeuristic(Graph* xgraph,
                               NodeMap<double>* xscore,
                               Node* xroot,
                               int xsize,
                               set<Node>* xexclude,
                               set<Node>* xreceptors)
 : graph { xgraph },
   score { xscore },
   root { xroot },
   size { xsize },
   exclude { xexclude },
   receptors { xreceptors }
{
    start_solution = new set<Node>();
}

bool StartHeuristic::run() {
    if (!root)
        root = getMaximalRoot();
    if (!root)
        return false;
    start_solution->insert(*root);
    Node* next;
    while (--size > 0) {
        next = argmax();
        if ( next )
            start_solution->insert(*next);
        else
            return false;
    }
    return true;
}

pair<Node, set<Node>>* StartHeuristic::getStartSolution() {
    pair<Node, set<Node>>* ret { new pair<Node, set<Node>>( make_pair(*root, *start_solution) ) };
    return ret;
}

Node* StartHeuristic::getMaximalRoot() {
    double max;
    Node* argmax { nullptr };
    if (receptors)
        for (auto v : *receptors)
            update_max(&argmax, &v, &max);
    else
        for (NodeIt v(*graph); v != INVALID; ++v)
            update_max(&argmax, &v, &max);
    return argmax;
}

Node* StartHeuristic::argmax() {
    double max;
    Node* argmax { nullptr };
    Node* u = new Node();
    for (auto v : *start_solution) {
        for (OutArcIt a(*graph, v); a != INVALID; ++a) {
            *u = graph->target(a);
            if (exclude) {
                if (start_solution->find(*u) == start_solution->end() && exclude->find(*u) == exclude->end())
                    update_max(&argmax, u, &max);
            }
            else
                if (start_solution->find(*u) == start_solution->end())
                    update_max(&argmax, u, &max);
        }
    }
    return argmax;
}

void StartHeuristic::update_max(Node** argmaxp, Node* node, double* maxp) {
    if (!(*argmaxp)) {
        *argmaxp = new Node();
        **argmaxp = *node;
        *maxp = (*score)[*node];
    }
    else if ((*score)[*node] > *maxp) {
        **argmaxp = *node;
        *maxp = (*score)[*node];
    }
}

}    //    namespace deregnet
