#include "markov_algorithm.h"

#include <iostream>
#include <fstream>


template <class T>
void dprint(T&& t, std::string file = "debug_output.txt")
{
    std::ofstream out(file, std::fstream::app);
    out << t << std::endl;
    out.close();
}

int* markov_algorithm::get_brinks(std::string p)
{
    int n = p.length();
    int* br = new int[n];
    br[0] = 0;
    int k = 0;
    for (int q = 1; q < n; q++)
    {
        while (k > 0 && p[k] != p[q])
            k = br[k - 1];
        if (p[k] == p[q])
            ++k;
        br[q] = k;
    }
    return br;
}
int markov_algorithm::get_substrings(std::string t, event_sequence& subst)
{
    std::string p = "";
    int res = 0;
    for (int i = 0; i < subst.seq.size(); ++i)
        p += subst.seq[i]->type;
    int n = t.length();
    int m = p.length();
    int* br = get_brinks(p);
    int q = 0;
    for (int i = 0; i < n; i++)
    {
        while (q > 0 && p[q] != t[i])
            q = br[q - 1];
        if (p[q] == t[i])
            ++q;
        if (q == m)
        {
            //res.push_back(i - m + 1);
            res++;
            q = br[q - 1];
        }
    }
    return res;
}

void markov_algorithm::rec(const std::vector<event_type_ptr> & al, std::vector<int>::iterator begin, std::vector<int>::iterator pos, std::vector<int>::iterator end, std::set<event_sequence>& es)
{
    //cout << *pos << " " << *end << endl;
    if (pos >= end)
    {
        std::vector<event_type_ptr> a;
        for (auto i = begin; i != end; i++)
            a.push_back(al[*i]);
        es.insert(event_sequence(move(a)));
        return;
    }
    else
    {
        for (int i = 0; i < al.size(); i++)
        {
            *pos = i;
            rec(al, begin, pos + 1, end, es);
        }
    }
}
void markov_algorithm::get_all_subseq(std::string& trace, int n, std::vector<event_sequence>& es, std::vector<event_type_ptr>& al)
{
    std::set<event_type> et;
    std::set<event_type_ptr> temp;
    std::set<event_sequence> es1;

    for (int i = 0; i < trace.size(); i++)
        et.insert(event_type(trace.substr(i, 1)));
    for (auto& r : et)
        temp.insert(event_type_ptr(new event_type(r)));

    al = std::vector<event_type_ptr>(temp.begin(), temp.end());

    for (int i = 1; i <= n; i++)
    {
        std::vector<int> temp1(i);
        rec(al, temp1.begin(), temp1.begin(), temp1.end(), es1);
    }

    //cout << "tram" << endl;
    es = std::vector<event_sequence>(es1.begin(), es1.end());
}

void markov_algorithm::set_count_and_prob(std::string& trace, std::vector<event_sequence>& seqs)
{
    int currentBase = 1;
    int currentSecondBase = 1;
    for (event_sequence& s : seqs)
    {
        s.count = get_substrings(trace, s);
        if (s.seq.size() == 1) //TODO: ������ �������
            currentBase = s.count;
        if (s.seq.size() == 2)
        {
            s.probability = (double)s.count / currentBase;
            currentSecondBase = s.count;
        }
        else
        {
            if (currentSecondBase == 0)
                s.probability = 0;
            else
                s.probability = (double)s.count / currentSecondBase;
        }
        //cout << s << "->" << s.count << ", " << s.probability << endl;
    }
}
void markov_algorithm::build_graph()
{
    for (event_sequence& es : all_event_seq)
    {
        if (es.probability > 0)
        {
            for (auto it = es.seq.begin(), nit = ++es.seq.begin(); nit != es.seq.end(); it++, nit++)
                event_graph.add_edge(*it, *nit);
        }
    }
}
bool markov_algorithm::resolve(vertex_sequence & illegal_sequence)
{
    std::cout << "Entered resolve: " << illegal_sequence << std::endl;
    try
    {
        vertex splitting_v = iterate_modifications(illegal_sequence);
        return true;
    }
    catch (std::exception e)
    {
        std::cout << e.what();
        return false;
    }
}
bool markov_algorithm::_try_split(vertex_sequence& illegal_seq, vertex sv, int pos)
{
    vertex_sequence before;
    vertex_sequence after;

    if (!(illegal_seq.is_valid(all_event_seq, 0, pos + 1, probability_thd, count_thd) || !(illegal_seq.is_valid(all_event_seq, pos, illegal_seq.get_size(), probability_thd, count_thd))))
        return false;

    std::cout << "Yay! Found split for vertex " << sv << std::endl;

    edge_set incident_edges = event_graph[sv];
    edge_set new_edges;
    vertex new_vertex(sv, sv.id);

    edge in;
    edge out;
    std::cout << "Incident edges: " << std::endl;
    for (auto& e : incident_edges)
    {
        if (e.first == illegal_seq[pos - 1])
            in = e;
        if (e.second == illegal_seq[pos + 1])
            out = e;
        std::cout << e.first << "->" << e.second << std::endl;
    }
    incident_edges.erase(in);
    incident_edges.erase(out);

    std::cout << "w/o illegal in/out" << std::endl;
    for (auto& e : incident_edges)
        std::cout << e.first << "->" << e.second << std::endl;

    event_graph.remove_edge(out);

    for (auto& e : incident_edges)
    {
        if (e.first == illegal_seq[pos])
            new_edges.insert(std::make_pair(new_vertex, e.second));
        if (e.second == illegal_seq[pos])
            new_edges.insert(std::make_pair(e.first, new_vertex));
    }

    new_edges.insert(std::make_pair(new_vertex, out.second));
    std::cout << "Constructed new edges" << std::endl;
    for (auto& e : new_edges)
    {
        std::cout << e.first << "->" << e.second << std::endl;
        event_graph.insert_edge(e);
    }

    dprint(event_graph);
    /*std::cout << "Graph" << std::endl;
    std::cout << event_graph << std::endl;
    std::cout << std::endl;*/
    return true;
    //std::set_union(_edges.begin(), _edges.end(), new_edges.begin(), new_edges.end(), incident_edges);
}

vertex markov_algorithm::iterate_modifications(vertex_sequence& illegal_seq)
{
    
    for (int i = 1; i<illegal_seq.get_size() - 1; i++)
    {
        if (_try_split(illegal_seq, illegal_seq[i], i))
            return illegal_seq[i];
    }
    throw std::exception("Nothing found!"); // ���� ��������� ��� ��������� � �� ����� �������� ��������� ������������������
}
void markov_algorithm::do_magic()
{
    sort(all_event_seq.begin(), all_event_seq.end());
    //std::cout << "1" << std::endl;
    for (auto& el : event_graph) //�� �����, ���� - (������� : ������ ����������� ��)
    {
        std::vector<vertex_sequence> vs = event_graph.dfs_stack(3, el.first); //��� ������� ������� ������� ��� ���� ����� 3
        for (auto& path : vs) // ����������� �� �����, ��������� �� ����������
        {
            //std::cout << path << std::endl;
            if (do_split && !(path.is_valid(all_event_seq, 0, all_event_seq.size(), probability_thd, count_thd)))
            {
                bool flag = resolve(path);
                std::cout << "resolve returned: " << flag << std::endl;
            }
        }
    }

}