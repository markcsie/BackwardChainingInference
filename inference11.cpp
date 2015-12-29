#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

class Literal {
public:

    Literal() : predicate("") {

    }

    explicit Literal(const std::string &raw_literal) {
        std::size_t found = raw_literal.find("(");
        predicate = raw_literal.substr(0, found);

        std::string argument = "";
        for (char c : raw_literal.substr(found + 1)) {
            if (c == ',' || c == ')') {
                argumentVector.push_back(argument);
                argument = "";
            } else {
                argument += c;
            }
        }
    }

    std::string GetPredicate() const {
        return predicate;
    }

    bool operator==(const Literal &rhs) {
        if (predicate != rhs.predicate) {
            return false;
        }
        if (argumentVector.size() != rhs.argumentVector.size()) {
            return false;
        }
        for (unsigned int i = 0; i < argumentVector.size(); ++i) {
            if (argumentVector[i] != rhs.argumentVector[i]) {
                return false;
            }
        }
        return true;
    }

    void SetArgumentValue(const unsigned int &i, const std::string &value) {
        argumentVector[i] = value;
    }

    void SetArgumentName(const unsigned int &i, const std::string &name) {
        argumentVector[i] = name;
    }

    std::vector<std::string> GetArgumentVector() const {
        return argumentVector;
    }

private:
    friend std::ostream& operator<<(std::ostream &os, const Literal &l);

    std::string predicate;
    std::vector<std::string> argumentVector;
};

std::ostream& operator<<(std::ostream &os, const Literal &l) {
    os << l.predicate << "(";
    for (auto i = l.argumentVector.begin(); i != l.argumentVector.end(); ++i) {
        if (i != l.argumentVector.begin()) {
            os << ",";
        }
        os << *i;
    }
    os << ")";
    return os;
}

class Sentence {
public:

    Sentence(const std::string &raw_lhs, const std::string &raw_rhs) {
        std::string raw_literal = "";
        for (unsigned int i = 0; i < raw_lhs.length(); ++i) {
            if (raw_lhs[i] == '^' || i == raw_lhs.length() - 1) {
                Literal literal(raw_literal);
                lhs.push_back(literal);
                raw_literal.clear();
                ++i;
            } else {
                raw_literal += raw_lhs[i];
            }
        }
        rhs = Literal(raw_rhs);
    }

    bool GoalMatching(const Literal &goal) const {
        return goal.GetPredicate() == rhs.GetPredicate();
    }

    // Note: In this assignment, variables are all single lowercase letters

    Sentence Standardize() const {
        Sentence s = *this;
        for (unsigned int i = 0; i < s.lhs.size(); ++i) {
            for (unsigned int j = 0; j < s.lhs[i].GetArgumentVector().size(); ++j) {
                std::string variable = s.lhs[i].GetArgumentVector()[j];
                if (variable.length() > 1 || !std::islower(variable[0])) {
                    continue;
                }
                std::ostringstream variable_count_ss;
                variable_count_ss << ++Sentence::variable_count;
                s.lhs[i].SetArgumentName(j, variable + variable_count_ss.str());

                // lhs
                for (unsigned int k = i; k < s.lhs.size(); ++k) {
                    for (unsigned int l = 0; l < s.lhs[k].GetArgumentVector().size(); ++l) {
                        std::string compared_variable = s.lhs[k].GetArgumentVector()[l];
                        if (variable == compared_variable) {
                            s.lhs[k].SetArgumentName(l, compared_variable + variable_count_ss.str());
                        }
                    }
                }

                // rhs
                std::vector<std::string> argument_vector = s.rhs.GetArgumentVector();
                for (unsigned int k = 0; k < argument_vector.size(); ++k) {
                    std::string compared_variable = argument_vector[k];
                    if (variable == compared_variable) {
                        s.rhs.SetArgumentName(k, compared_variable + variable_count_ss.str());
                    }
                }
            }
        }

        // rhs
        for (unsigned int k = 0; k < s.rhs.GetArgumentVector().size(); ++k) {
            std::string variable = s.rhs.GetArgumentVector()[k];
            if (variable.length() == 1 && std::islower(variable[0])) {
                std::ostringstream variable_count_ss;
                variable_count_ss << ++Sentence::variable_count;
                
                for (unsigned int l = k+1; l < s.rhs.GetArgumentVector().size(); ++l) {
                    std::string compared_variable = s.rhs.GetArgumentVector()[l];
                    if (variable == compared_variable) {
                        s.rhs.SetArgumentName(l, variable + variable_count_ss.str());
                    }
                }

                s.rhs.SetArgumentName(k, variable + variable_count_ss.str());
            }
        }

        return s;
    }
    
    static void ResetVariableCount() {
        Sentence::variable_count = 0;
    }

    std::vector<Literal> GetLhs() const {
        return lhs;
    }

    Literal GetRhs() const {
        return rhs;
    }

private:
    std::vector<Literal> lhs;
    Literal rhs; // NOTE: only one literal in this assignment???

    friend std::ostream & operator<<(std::ostream &os, const Sentence &s);

    static unsigned long long int variable_count;
};
unsigned long long int Sentence::variable_count = 0;

std::ostream & operator<<(std::ostream &os, const Sentence &s) {
    if (!s.lhs.empty()) {
        for (auto i = s.lhs.begin(); i != s.lhs.end(); ++i) {
            if (i != s.lhs.begin()) {
                os << " ^ ";
            }
            os << *i;
        }
        os << " => ";
    }
    os << s.rhs;
    return os;
}

class Substitutions {
public:

    void SetFailed() {
        pairs.insert(failed_pair);
    }

    bool IsFailed() const {
        if (pairs.find("failure") != pairs.end()) {
            return true;
        }
        return false;
    }

    Literal Substitute(const Literal &x) const {
        Literal literal = x;
        std::vector<std::string> argument_vector = x.GetArgumentVector();
        for (unsigned i = 0; i < argument_vector.size(); ++i) {
            auto found = pairs.find(argument_vector[i]);
            if (found != pairs.end()) {
                literal.SetArgumentValue(i, found->second);
            }
        }
        return literal;
    }

    void AddPair(const std::string &var, const std::string &value) {
        pairs.insert({var, value});
    }

    std::string Find(const std::string &var) const {
        auto found = pairs.find(var);
        if (found != pairs.end()) {
            return found->second;
        }
        return "";
    }

private:
    static const std::pair<std::string, std::string> failed_pair;
    std::unordered_map<std::string, std::string> pairs;

    friend std::ostream & operator<<(std::ostream &os, const Substitutions &s);
};
const std::pair<std::string, std::string> Substitutions::failed_pair = std::make_pair("failure", "failure");

std::ostream & operator<<(std::ostream &os, const Substitutions &s) {
    os << "{";
    for (auto i = s.pairs.begin(); i != s.pairs.end(); ++i) {
        if (i != s.pairs.begin()) {
            os << ", ";
        }
        os << i->first << '/' << i->second;
    }
    os << "}";
    return os;
}

class KnowledgeBase {
public:

    void AddSentence(const std::string &raw_sentence) {
        std::string lhs = "";
        std::string rhs = "";

        std::size_t found = raw_sentence.find("=>");
        if (found != std::string::npos) {
            lhs = raw_sentence.substr(0, found);
            rhs = raw_sentence.substr(found + 3);
        } else { // facts
            rhs = raw_sentence;
        }

        Sentence sentence(lhs, rhs);
        sentences.push_back(sentence);
    }

    Substitutions UnifyVar(const std::string &var, const std::string &x, Substitutions substitutions) const {
        std::string value = substitutions.Find(var);
        if (value != "") {
            return Unify(value, x, substitutions);
        }
        value = substitutions.Find(x);
        if (value != "") {
            return Unify(var, value, substitutions);
        }
        // NOTE: In this assignment, we don't have functions, so no occur-check.

        substitutions.AddPair(var, x);
        return substitutions;
    }

    Substitutions Unify(const std::string &x, const std::string &y, Substitutions substitutions) const {
        if (substitutions.IsFailed() || x == y) {
            return substitutions;
        } else if (std::islower(x[0])) { // x is a variable
            return UnifyVar(x, y, substitutions);
        } else if (std::islower(y[0])) { // y is a variable
            return UnifyVar(y, x, substitutions);
        } else {
            substitutions.SetFailed();
            return substitutions;
        }
    }

    Substitutions Unify(const std::vector<std::string> &x, const std::vector<std::string> &y, Substitutions substitutions) const {
        if (substitutions.IsFailed() || x == y) {
            return substitutions;
        } else {
            std::vector<std::string> rest_x(x.begin() + 1, x.end());
            std::vector<std::string> rest_y(y.begin() + 1, y.end());
            return Unify(rest_x, rest_y, Unify(x[0], y[0], substitutions));
        }
    }

    // NOTE: In this assignment, we don't have to find all of the possible substitutions

    Substitutions BackwardChainingAnd(const std::vector<Literal> &goals, Substitutions substitutions, std::unordered_map<std::string, std::string> explored_map) {
        if (substitutions.IsFailed()) {
            return substitutions;
        } else if (goals.empty()) {
            return substitutions;
        } else {
            std::vector<Substitutions> possible_substitutions = BackwardChainingOr(substitutions.Substitute(goals.front()), substitutions, explored_map);
            for (Substitutions s : possible_substitutions) {
                std::vector<Literal> rest_goals(goals.begin() + 1, goals.end());
                Substitutions new_substitutions = BackwardChainingAnd(rest_goals, s, explored_map);
                if (!new_substitutions.IsFailed()) {
                    return new_substitutions;
                }
            }
            substitutions.SetFailed();
            return substitutions;
        }
    }

    std::vector<Substitutions> BackwardChainingOr(const Literal &goal, Substitutions substitutions, std::unordered_map<std::string, std::string> explored_map) {
        std::vector<Substitutions> possible_substitutions;
        std::stringstream ss;
        ss << goal;
        // NOTE: In this assignment, if the goal contains variables, we would not get a loop
//                std::cout << goal << std::endl;
        if (explored_map.find(ss.str()) == explored_map.end()) {
            explored_map.insert({ss.str(), ss.str()});
            for (Sentence s : GetRulesForGoal(goal)) {
                Sentence standardized_s = s.Standardize();
                //                std::cout << s << std::endl;
                                std::cout << standardized_s << std::endl;
                Substitutions new_substitutions = BackwardChainingAnd(standardized_s.GetLhs(), Unify(standardized_s.GetRhs().GetArgumentVector(), goal.GetArgumentVector(), substitutions), explored_map);
                if (!new_substitutions.IsFailed()) {
                    possible_substitutions.push_back(new_substitutions);
                    // NOTE: Sentence proved
                    //                    std::cout << standardized_s << std::endl;
                    //                    std::cout << new_substitutions << std::endl;
                }
            }
        }
        return possible_substitutions;
    }

    bool BackwardChainingAsk(const Literal &query) {
        Sentence::ResetVariableCount();
        Substitutions substitutions;
        std::unordered_map<std::string, std::string> explored_map;
        std::vector<Substitutions> answer_substitutions = BackwardChainingOr(query, substitutions, explored_map);
        for (Substitutions s : answer_substitutions) {
            if (!s.IsFailed()) {
                return true; // NOTE: The substitution variables would be different
            }
        }
        return false;
    }

private:
    std::vector<Sentence> sentences;

    friend std::ostream & operator<<(std::ostream &os, const KnowledgeBase &kb);

    std::vector<Sentence> GetRulesForGoal(const Literal &goal) {
        std::vector<Sentence> goal_sentences;

        for (Sentence s : sentences) {
            if (s.GoalMatching(goal)) {
                goal_sentences.push_back(s);
            }
        }
        return goal_sentences;
    }
};

std::ostream & operator<<(std::ostream &os, const KnowledgeBase &kb) {
    for (Sentence s : kb.sentences) {
        os << s << std::endl;
    }
    return os;
}

int main(int argc, char** argv) {
    std::ifstream infile(argv[2]);

    std::string line;

    // Queries
    std::vector<Literal> queries;
    std::getline(infile, line);
    unsigned int numOfQueries = std::stoi(line);
    std::string raw_query;
    for (unsigned int i = 0; i < numOfQueries; ++i) {
        std::getline(infile, raw_query);
        Literal query(raw_query);
        queries.push_back(query);
    }

    // KB
    KnowledgeBase kb;
    std::getline(infile, line);
    unsigned int numOfClauses = std::stoi(line);
    std::string clause;
    for (unsigned int i = 0; i < numOfClauses; ++i) {
        std::getline(infile, clause);
        kb.AddSentence(clause);
    }
    std::cout << "KB:" << std::endl;
    std::cout << kb;

    std::ofstream output_file;
    output_file.open("output.txt");

    std::cout << std::endl << "Queries:" << std::endl;
    for (Literal q : queries) {
        std::cout << q << std::endl;
        if (kb.BackwardChainingAsk(q)) {
            output_file << "TRUE" << std::endl;
            std::cout << "TRUE" << std::endl;
        } else {
            output_file << "FALSE" << std::endl;
            std::cout << "FALSE" << std::endl;
        }
        output_file.flush();
        std::cout << std::endl;
    }
    output_file.close();
    return 0;
}


