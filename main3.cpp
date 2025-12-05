//cd "/Users/bhaskarkhadka/Desktop/HW/llcv/" && g++ -std=c++17 main3.cpp -o main3 && "/Users/bhaskarkhadka/Desktop/HW/llcv/"main3

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <limits>
#include <cmath>
#include <map>
#include <unordered_map>
#include <set>
#include <chrono>
using namespace std;

// -----------------------------
// Config / DB file names
// -----------------------------
const string DB_FILE = "database_main3.txt";
const string TX_CSV = "transactions_export_main3.csv";
const string MONTH_CSV = "monthly_export_main3.csv";

const string ACC_SECTION = "#ACCOUNTS";
const string TX_SECTION  = "#TRANSACTIONS";
const string MONTH_SECTION = "#MONTHLY";
const string BUDGET_SECTION = "#BUDGETS";

// Default categories
vector<string> default_income_categories = {
    "Salary","Business","Freelance","Dividends","Rental","Interest","Bonus","Other"
};

vector<string> default_expense_categories = {
    "Food","Rent","Utilities","Transport","Shopping","Bills","Entertainment","Other"
};


static inline string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static inline vector<string> split(const string &s, char delim) {
    vector<string> out;
    string token;
    istringstream iss(s);
    while (getline(iss, token, delim)) out.push_back(token);
    return out;
}

string nowTimestamp() {
    time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
    tm *tm_ptr = &tm_buf;
#else
    tm *tm_ptr = localtime(&t);
#endif
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_ptr);
    return string(buf);
}

string todayDate() {
    time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
    tm *tm_ptr = &tm_buf;
#else
    tm *tm_ptr = localtime(&t);
#endif
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d", tm_ptr);
    return string(buf);
}

// Wait for enter
void pressEnter() {
    cout << "\nPress Enter to continue...";
    string tmp;
    getline(cin, tmp);
}

string inputLine(const string &prompt, bool allowEmpty=false) {
    cout << prompt;
    string s;
    getline(cin, s);
    s = trim(s);
    while (!allowEmpty && s.empty()) {
        cout << "Input cannot be empty. " << prompt;
        getline(cin, s);
        s = trim(s);
    }
    return s;
}

int inputInt(const string &prompt, int def=-1) {
    while (true) {
        string s = inputLine(prompt, def != -1);
        if (s.empty()) return def;
        try {
            int v = stoi(s);
            return v;
        } catch(...) {
            cout << "Please enter a valid integer.\n";
        }
    }
}

double inputDouble(const string &prompt, double def = numeric_limits<double>::quiet_NaN()) {
    while (true) {
        string s = inputLine(prompt, !(std::isnan(def)));
        if (s.empty()) {
            if (!std::isnan(def)) return def;
            cout << "Value cannot be empty.\n";
            continue;
        }
        try {
            double v = stod(s);
            return v;
        } catch(...) {
            cout << "Please enter a valid number.\n";
        }
    }
}

bool confirm(const string &prompt) {
    while (true) {
        string s = inputLine(prompt + " (y/n): ");
        for (auto &c : s) c = tolower((unsigned char)c);
        if (s == "y" || s == "yes") return true;
        if (s == "n" || s == "no") return false;
        cout << "Please enter y or n.\n";
    }
}

struct Account {
    string acctNo;
    string username;
    string firstName;
    string lastName;
    string email;
    string phone;
    string pin; 
    double balance = 0.0;
};

struct Transaction {
    string timestamp; 
    string acctNo;
    string type; 
    double amount;
    string category;
    string note;
    double balanceAfter;
};

struct MonthlyRecord {
    string acctNo;
    int year;
    int month; 
    double salary;
    double food;
    double rent;
    double other;
    double savings;
};

struct BudgetRecord {
    string acctNo;
    int year;
    int month;
    map<string,double> budgets; 
};


vector<Account> accounts;
vector<Transaction> transactions;
vector<MonthlyRecord> monthlyData;
vector<BudgetRecord> budgets;
vector<string> incomeCategories = default_income_categories;
vector<string> expenseCategories = default_expense_categories;


void saveDatabase();

void ensureDbFile() {
    ifstream in(DB_FILE);
    if (in) return;
    ofstream out(DB_FILE);
    out << ACC_SECTION << "\n";
    out << "ADMIN|admin|Admin|User|admin@example.com|000-000-0000|1234|0.00\n";
    out << "\n" << TX_SECTION << "\n\n";
    out << MONTH_SECTION << "\n\n";
    out << BUDGET_SECTION << "\n\n";
    out.close();
}

string accountToLine(const Account &a) {
    ostringstream oss;
    oss << a.username << "|" << a.acctNo << "|" << a.firstName << "|" << a.lastName
        << "|" << a.email << "|" << a.phone << "|" << a.pin << "|" << fixed << setprecision(2) << a.balance;
    return oss.str();
}

bool lineToAccount(const string &line, Account &a) {
    auto parts = split(line, '|');
    if (parts.size() < 8) return false;
    a.username = trim(parts[0]);
    a.acctNo = trim(parts[1]);
    a.firstName = trim(parts[2]);
    a.lastName = trim(parts[3]);
    a.email = trim(parts[4]);
    a.phone = trim(parts[5]);
    a.pin = trim(parts[6]);
    try { a.balance = stod(trim(parts[7])); } catch(...) { a.balance = 0.0; }
    return true;
}

string txToLine(const Transaction &t) {
    ostringstream oss;
    oss << t.timestamp << "|" << t.acctNo << "|" << t.type << "|" << fixed << setprecision(2) << t.amount
        << "|" << t.category << "|" << t.note << "|" << fixed << setprecision(2) << t.balanceAfter;
    return oss.str();
}

bool lineToTx(const string &line, Transaction &t) {
    auto parts = split(line, '|');
    if (parts.size() < 7) return false;
    t.timestamp = trim(parts[0]);
    t.acctNo = trim(parts[1]);
    t.type = trim(parts[2]);
    try { t.amount = stod(trim(parts[3])); } catch(...) { t.amount = 0.0; }
    t.category = trim(parts[4]);
    t.note = trim(parts[5]);
    try { t.balanceAfter = stod(trim(parts[6])); } catch(...) { t.balanceAfter = 0.0; }
    return true;
}

string monthlyToLine(const MonthlyRecord &m) {
    ostringstream oss;
    oss << m.acctNo << "|" << m.year << "|" << m.month << "|" << fixed << setprecision(2) << m.salary
        << "|" << m.food << "|" << m.rent << "|" << m.other << "|" << m.savings;
    return oss.str();
}

bool lineToMonthly(const string &line, MonthlyRecord &m) {
    auto p = split(line, '|'); if (p.size() < 8) return false;
    m.acctNo = trim(p[0]);
    try { m.year = stoi(trim(p[1])); } catch(...) { m.year = 0; }
    try { m.month = stoi(trim(p[2])); } catch(...) { m.month = 0; }
    try { m.salary = stod(trim(p[3])); } catch(...) { m.salary = 0; }
    try { m.food = stod(trim(p[4])); } catch(...) { m.food = 0; }
    try { m.rent = stod(trim(p[5])); } catch(...) { m.rent = 0; }
    try { m.other = stod(trim(p[6])); } catch(...) { m.other = 0; }
    try { m.savings = stod(trim(p[7])); } catch(...) { m.savings = 0; }
    return true;
}

string budgetToLine(const BudgetRecord &b) {
    ostringstream oss;
    oss << b.acctNo << "|" << b.year << "|" << b.month << "|";
    bool first = true;
    for (auto &kv : b.budgets) {
        if (!first) oss << ",";
        first = false;
        oss << kv.first << ":" << kv.second;
    }
    return oss.str();
}

bool lineToBudget(const string &line, BudgetRecord &b) {
    auto p = split(line, '|'); if (p.size() < 4) return false;
    b.acctNo = trim(p[0]);
    try { b.year = stoi(trim(p[1])); } catch(...) { b.year = 0; }
    try { b.month = stoi(trim(p[2])); } catch(...) { b.month = 0; }
    b.budgets.clear();
    string rest = (p.size() >= 4) ? p[3] : string();
    if (rest.empty()) return true;
    auto pairs = split(rest, ',');
    for (auto &pr : pairs) {
        auto kv = split(pr, ':');
        if (kv.size() == 2) {
            string cat = trim(kv[0]);
            double val = 0.0;
            try { val = stod(trim(kv[1])); } catch(...) { val = 0.0; }
            if (!cat.empty()) b.budgets[cat] = val;
        }
    }
    return true;
}



void saveDatabase() {
    ofstream out(DB_FILE, ios::trunc);
    if (!out) { cerr << "Unable to write DB file.\n"; return; }
    out << ACC_SECTION << "\n";
    for (auto &a : accounts) out << accountToLine(a) << "\n";
    out << "\n" << TX_SECTION << "\n";
    for (auto &t : transactions) out << txToLine(t) << "\n";
    out << "\n" << MONTH_SECTION << "\n";
    for (auto &m : monthlyData) out << monthlyToLine(m) << "\n";
    out << "\n" << BUDGET_SECTION << "\n";
    for (auto &b : budgets) out << budgetToLine(b) << "\n";
    out.close();
}

Account* findAccountByUsername(const string &u) {
    for (auto &a : accounts) if (a.username == u) return &a;
    return nullptr;
}
Account* findAccountByAcctNo(const string &no) {
    for (auto &a : accounts) if (a.acctNo == no) return &a;
    return nullptr;
}

string genNextAccountNo() {
    int maxSeq = 0;
    for (auto &a : accounts) {
        if (a.acctNo.rfind("909-", 0) == 0) {
            try {
                int seq = stoi(a.acctNo.substr(4));
                maxSeq = max(maxSeq, seq);
            } catch(...) {}
        }
    }
    int next = maxSeq + 1;
    ostringstream oss; oss << "909-" << setw(2) << setfill('0') << next;
    return oss.str();
}

void appendTransaction(const Transaction &t) {
    transactions.push_back(t);
    saveDatabase();
}

void exportTransactionsCSV(const string &filename) {
    ofstream out(filename);
    if (!out) { cout << "Unable to create file " << filename << "\n"; return; }
    out << "timestamp,acctNo,type,amount,category,note,balanceAfter\n";
    for (auto &t : transactions) {
        out << "\"" << t.timestamp << "\"," << t.acctNo << "," << t.type << "," << fixed << setprecision(2) << t.amount
            << ",\"" << t.category << "\",\"" << t.note << "\"," << fixed << setprecision(2) << t.balanceAfter << "\n";
    }
    out.close();
    cout << "Transactions exported to " << filename << "\n";
}

void exportMonthlyCSV(const string &filename) {
    ofstream out(filename);
    if (!out) { cout << "Unable to create file " << filename << "\n"; return; }
    out << "acctNo,year,month,salary,food,rent,other,savings\n";
    for (auto &m : monthlyData) {
        out << m.acctNo << "," << m.year << "," << m.month << "," << fixed << setprecision(2)
            << m.salary << "," << m.food << "," << m.rent << "," << m.other << "," << m.savings << "\n";
    }
    out.close();
    cout << "Monthly data exported to " << filename << "\n";
}


void printBarChart(const map<string,double> &vals, int width=40) {
    double total = 0.0;
    for (auto &kv : vals) total += kv.second;
    if (total <= 0.0) { cout << "(no data)\n"; return; }
    for (auto &kv : vals) {
        double v = kv.second;
        int len = (int)round((v / total) * width);
        cout << setw(12) << left << kv.first << " | ";
        for (int i=0;i<len;i++) cout << '#';
        cout << " " << fixed << setprecision(2) << v << "\n";
    }
}

void createAccountFlow() {
    cout << "\nCREATE ACCOUNT\n";
    string username = inputLine("Username: ");
    if (findAccountByUsername(username)) { cout << "Username exists.\n"; return; }
    string first = inputLine("First name: ");
    string last = inputLine("Last name: ");
    string email = inputLine("Email: ");
    string phone = inputLine("Phone: ");
    string pin;
    while (true) {
        pin = inputLine("Create 4-digit PIN: ");
        if (pin.size() == 4 && all_of(pin.begin(), pin.end(), ::isdigit)) break;
        cout << "PIN must be exactly 4 digits.\n";
    }
    Account a;
    a.username = username;
    a.firstName = first;
    a.lastName = last;
    a.email = email;
    a.phone = phone;
    a.pin = pin;
    a.acctNo = genNextAccountNo();
    a.balance = 0.0;
    accounts.push_back(a);
    saveDatabase();
    cout << "Created account " << a.acctNo << " for " << a.firstName << " " << a.lastName << "\n";
}

string chooseCategoryForDeposit() {
    cout << "\nCategories:\n";
    for (size_t i=0;i<incomeCategories.size();++i) cout << i+1 << ". " << incomeCategories[i] << "\n";
    string input = inputLine("Choose by number or type new category: ", true);
    if (input.empty()) return "Other";
    if (!input.empty() && isdigit((unsigned char)input[0])) {
        try {
            int idx = stoi(input);
            if (idx >= 1 && idx <= (int)incomeCategories.size()) return incomeCategories[idx-1];
        } catch(...) {}
        return "Other";
    } else {
        // add custom
        if (find(incomeCategories.begin(), incomeCategories.end(), input) == incomeCategories.end()) incomeCategories.push_back(input);
        return input;
    }
}

string chooseCategoryForExpense() {
    cout << "\nCategories:\n";
    for (size_t i=0;i<expenseCategories.size();++i) cout << i+1 << ". " << expenseCategories[i] << "\n";
    string input = inputLine("Choose by number or type new category: ", true);
    if (input.empty()) return "Other";
    if (!input.empty() && isdigit((unsigned char)input[0])) {
        try {
            int idx = stoi(input);
            if (idx >= 1 && idx <= (int)expenseCategories.size()) return expenseCategories[idx-1];
        } catch(...) {}
        return "Other";
    } else {
        // add custom
        if (find(expenseCategories.begin(), expenseCategories.end(), input) == expenseCategories.end()) expenseCategories.push_back(input);
        return input;
    }
}

void depositFlow(Account &acc) {
    double amt = inputDouble("\nDeposit amount: ");
    if (amt <= 0.0) { cout << "Amount must be > 0.\n"; return; }
    string cat = chooseCategoryForDeposit();
    string note = inputLine("Note (optional): ", true);
    acc.balance += amt;
    Transaction t;
    t.timestamp = nowTimestamp();
    t.acctNo = acc.acctNo;
    t.type = "DEPOSIT";
    t.amount = amt;
    t.category = cat;
    t.note = note;
    t.balanceAfter = acc.balance;
    transactions.push_back(t);
    for (auto &a : accounts) if (a.acctNo == acc.acctNo) { a.balance = acc.balance; break; }
    saveDatabase();
    cout << "Deposited $" << fixed << setprecision(2) << amt << ". New balance: $" << acc.balance << "\n";
}

void expenseFlow(Account &acc) {
    double amt = inputDouble("Expenses Amount: ");
    if (amt <= 0.0) { cout << "Amount must be > 0.\n"; return; }
    string cat = chooseCategoryForExpense();
    string note = inputLine("Note (optional): ", true);
    acc.balance -= amt;
    Transaction t;
    t.timestamp = nowTimestamp();
    t.acctNo = acc.acctNo;
    t.type = "WITHDRAW";
    t.amount = amt;
    t.category = cat;
    t.note = note;
    t.balanceAfter = acc.balance;
    transactions.push_back(t);
    for (auto &a : accounts) if (a.acctNo == acc.acctNo) { a.balance = acc.balance; break; }
    saveDatabase();
    cout << cat << " Expenses $" << fixed << setprecision(2) << amt << ". New balance: $" << acc.balance << "\n";
}

// -------------- 1. Account detail -----------------
void printAccountDetail(const Account &acc) {
    cout << "\n==== Account Detail =====\n";
    cout << "Account Number: " << acc.acctNo << "\n";
    cout << "Name: " << acc.firstName << " " << acc.lastName << "\n";
    cout << "Email: " << acc.email << "\n";
    cout << "Phone: " << acc.phone << "\n";
    cout << "-----------------------------\n";
}

// ----------------- 2.Balance ---------------------
void showBalance(const Account &acc) {
    cout << "\nAccount Number: " << acc.acctNo << "\n";
    cout << "Balance: $" << fixed << setprecision(2) << acc.balance << "\n";
    cout << "-----------------------------\n";
}

// ----------- 5. History ------------
vector<Transaction> getTransactionsForAccount(const string &acctNo) {
    vector<Transaction> out;
    for (auto &t : transactions) if (t.acctNo == acctNo) out.push_back(t);
    return out;
}

//----------------- 5.2 search / filter -----------------
void printTxRow(const Transaction &t) {
    cout << t.timestamp << " | " << setw(10) << left << t.type << " | $" << setw(8) << right << fixed << setprecision(2) << t.amount
         << " | " << setw(12) << left << t.category << " | " << t.note << " | Bal: $" << fixed << setprecision(2) << t.balanceAfter << "\n";
}

void viewHistoryMenu(const Account &acc) {
    auto list = getTransactionsForAccount(acc.acctNo);
    if (list.empty()) {
        cout << "No transactions.\n";
        return;
    }

    while (true) {
        cout << "\nHistory Menu:\n1. Last 5\n2. Search/Filter\n3. Back\n\nChoose: ";
        int ch = inputInt("", -1);

        if (ch == 3) return;

        if (ch == 1) {
            // Always show last 5
            int N = 5;
            int start = max(0, (int)list.size() - N);
            for (int i = start; i < (int)list.size(); ++i)
                printTxRow(list[i]);

        }
        else if (ch == 2) {
                cout << "\nSearch Type:\n1. Deposits\n2. Expenses\n\nChoose: ";
                int tchoice = inputInt("", -1);

                string txType;
                vector<string> cats;

                if (tchoice == 1) {
                txType = "DEPOSIT";
                cats = incomeCategories;
                } 
                else if (tchoice == 2) {
                txType = "WITHDRAW";
                cats = expenseCategories;
                }
                else {
                cout << "Invalid.\n";
                continue;
                }

                cout << "\nSelect a category:\n";
                for (size_t i = 0; i < cats.size(); i++)
                    cout << i+1 << ". " << cats[i] << "\n";

                int csel = inputInt("Choose: ");
                if (csel < 1 || csel > (int)cats.size()) {
                    cout << "Invalid category.\n";
                    continue;
                }
                string chosenCat = cats[csel - 1];

                string from = inputLine("From date (YYYY-MM-DD, leave empty for all): ", true);
                string to   = inputLine("To date (YYYY-MM-DD, leave empty for all): ", true);

                vector<Transaction> res;

                for (auto &t : list) {
                
                    // match type
                    if (t.type != txType) continue;

                    // match category
                    if (t.category != chosenCat) continue;

                    // match date range
                    string d = t.timestamp.substr(0, 10); // YYYY-MM-DD
                    if (!from.empty() && d < from) continue;
                    if (!to.empty() && d > to) continue;

                    res.push_back(t);
                }

                if (res.empty()) {
                    cout << "No matching results.\n";
                    continue;
                }

                cout << "\nResults:\n";
                for (auto &t : res) printTxRow(t);
            }           
 
        else cout << "Invalid.\n";
    }
}

// -----------------------------
// Monthly & budgets
// -----------------------------
void saveOrUpdateMonthly(const MonthlyRecord &m) {
    for (auto &r : monthlyData) {
        if (r.acctNo == m.acctNo && r.year == m.year && r.month == m.month) { r = m; saveDatabase(); return; }
    }
    monthlyData.push_back(m);
    saveDatabase();
}

void monthlyEntryFlow(Account &acc) {
    int year = inputInt("Enter year (e.g., 2025): ");
    int month = inputInt("Enter month (1-12): ");
    if (month < 1 || month > 12) { cout << "Invalid month.\n"; return; }
    double salary = inputDouble("Salary: ");
    double food = inputDouble("Food: ");
    double rent = inputDouble("Rent: ");
    double other = inputDouble("Other: ");
    MonthlyRecord m; m.acctNo = acc.acctNo; m.year = year; m.month = month; m.salary = salary; m.food = food; m.rent = rent; m.other = other;
    m.savings = salary - (food + rent + other);
    saveOrUpdateMonthly(m);
    cout << "Saved. Savings: $" << fixed << setprecision(2) << m.savings << "\n";
}

void viewMonthlyReport(const string &acctNo) {
    vector<MonthlyRecord> list;
    for (auto &m : monthlyData) if (m.acctNo == acctNo) list.push_back(m);
    if (list.empty()) { cout << "No monthly data.\n"; return; }
    sort(list.begin(), list.end(), [](const MonthlyRecord &a, const MonthlyRecord &b){ if (a.year != b.year) return a.year < b.year; return a.month < b.month; });
    cout << "Month  | Salary    | Expenses  | Savings\n";
    cout << "--------------------------------------\n";
    for (auto &m : list) {
        double expenses = m.food + m.rent + m.other;
        cout << setw(2) << setfill('0') << m.month << "/" << m.year << setfill(' ')
             << " | $" << setw(8) << fixed << setprecision(2) << m.salary
             << " | $" << setw(8) << expenses
             << " | $" << setw(8) << m.savings << "\n";
    }
    if (confirm("Show ASCII breakdown for a month?")) {
        int y = inputInt("Year: "); int mm = inputInt("Month: ");
        MonthlyRecord *mr = nullptr;
        for (auto &m : monthlyData) if (m.acctNo == acctNo && m.year == y && m.month == mm) { mr = &m; break; }
        if (!mr) { cout << "No record.\n"; return; }
        map<string,double> vals; vals["Food"] = mr->food; vals["Rent"] = mr->rent; vals["Other"] = mr->other;
        printBarChart(vals);
    }
}

void setBudgets(const string &acctNo) {
    int year = inputInt("Year: ");
    int month = inputInt("Month: ");
    if (month < 1 || month > 12) { cout << "Invalid month.\n"; return; }
    BudgetRecord b; b.acctNo = acctNo; b.year = year; b.month = month;
    cout << "Enter budgets for categories. Leave empty to skip.\n";
    for (auto &cat : expenseCategories) {
        string s = inputLine(cat + ": $", true);
        if (s.empty()) continue;
        try { b.budgets[cat] = stod(s); } catch(...) { cout << "Invalid for " << cat << ", skipping.\n"; }
    }
    // save or update
    for (auto &r : budgets) {
        if (r.acctNo == b.acctNo && r.year == b.year && r.month == b.month) { r = b; saveDatabase(); cout << "Budget updated.\n"; return; }
    }
    budgets.push_back(b);
    saveDatabase();
    cout << "Budgets saved.\n";
}

void viewBudgetsUsage(const string &acctNo) {
    int year = inputInt("Year: ");
    int month = inputInt("Month: ");
    BudgetRecord *b = nullptr;
    for (auto &r : budgets) if (r.acctNo == acctNo && r.year == year && r.month == month) { b = &r; break; }
    if (!b) { cout << "No budgets for that month.\n"; return; }
    map<string,double> usage;
    ostringstream oss; oss << setw(4) << setfill('0') << year << "-" << setw(2) << setfill('0') << month;
    string yf = oss.str();
    for (auto &t : transactions) {
        if (t.acctNo != acctNo) continue;
        if (t.timestamp.substr(0,7) != yf) continue;
        if (t.type == "WITHDRAW") usage[t.category] += t.amount;
    }
    cout << "Budget | Used | Status\n";
    map<string,double> display;
    for (auto &kv : b->budgets) {
        double used = usage[kv.first];
        cout << setw(12) << left << kv.first << " | $" << setw(8) << right << fixed << setprecision(2) << kv.second
             << " | $" << setw(8) << used;
        if (used > kv.second) cout << "  <-- OVER by $" << fixed << setprecision(2) << (used - kv.second);
        cout << "\n";
        display[kv.first] = used;
    }
    cout << "\nASCII usage:\n";
    printBarChart(display);
}

//------------------- Admin ranking list ---------------------
void adminRankingMenu() {
    cout << "\nADMIN RANKINGS (Simple List)\n";
    cout << "1. Richest (DESC)\n2. Richest (ASC)\n3. Highest Spenders (DESC)\n4. Highest Spenders (ASC)\n5. Back\n";
    int ch = inputInt("Choose: ");
    if (ch == 5) return;
    if (ch == 1 || ch == 2) {
        vector<pair<string,double>> list;
        for (auto &a : accounts) list.push_back({a.acctNo, a.balance});
        if (ch == 1) sort(list.begin(), list.end(), [](auto &x, auto &y){ return x.second > y.second; });
        else sort(list.begin(), list.end(), [](auto &x, auto &y){ return x.second < y.second; });
        cout << "\nAccount | Name | Balance\n";
        cout << "---------------------------\n";
        for (auto &p : list) {
            Account *a = findAccountByAcctNo(p.first);
            if (a) cout << a->acctNo << " | " << a->firstName << " " << a->lastName << " | $" << fixed << setprecision(2) << a->balance << "\n";
        }
    } else if (ch == 3 || ch == 4) {
        // compute spend per account
        unordered_map<string,double> spent;
        for (auto &t : transactions) if (t.type == "WITHDRAW") spent[t.acctNo] += t.amount;
        vector<pair<string,double>> list(spent.begin(), spent.end());
        if (ch == 3) sort(list.begin(), list.end(), [](auto &x, auto &y){ return x.second > y.second; });
        else sort(list.begin(), list.end(), [](auto &x, auto &y){ return x.second < y.second; });
        cout << "\nAccount | Name | TotalSpent\n";
        cout << "---------------------------\n";
        for (auto &p : list) {
            Account *a = findAccountByAcctNo(p.first);
            if (a) cout << a->acctNo << " | " << a->firstName << " " << a->lastName << " | $" << fixed << setprecision(2) << p.second << "\n";
        }
    } else cout << "Invalid.\n";
    pressEnter();
}

// ---------------- Admin menu ----------------
void adminMenu() {
    string user = inputLine("Admin username: ");
    string pin = inputLine("Enter 4-digit PIN: ");
    Account *adm = findAccountByUsername(user);
    if (!adm || adm->pin != pin || adm->username != "admin") {
        cout << "Admin auth failed.\n"; return;
    }
    while (true) {
        cout << "\nADMIN MENU\n1. List accounts\n2. View account\n3. Delete account\n4. Reset PIN\n5. Export CSVs\n6. Rankings (Simple List)\n7. Back\n";
        int ch = inputInt("Choose: ");
        if (ch == 7) break;
        if (ch == 1) {
            cout << "\nAccounts:\n";
            for (auto &a : accounts) cout << a.acctNo << " | " << a.username << " | " << a.firstName << " " << a.lastName << " | $" << fixed << setprecision(2) << a.balance << "\n";
            pressEnter();
        } else if (ch == 2) {
            string who = inputLine("username or acctNo: ");
            Account *a = findAccountByUsername(who); if (!a) a = findAccountByAcctNo(who);
            if (!a) { cout << "Not found.\n"; continue; }
            printAccountDetail(*a);
            if (confirm("View last 5 transactions?")) {
                auto list = getTransactionsForAccount(a->acctNo);
                int start = max(0, (int)list.size() - 5);
                for (int i = start; i < (int)list.size(); ++i) printTxRow(list[i]);
            }
            if (confirm("View monthly reports?")) viewMonthlyReport(a->acctNo);
            pressEnter();
        } else if (ch == 3) {
            string who = inputLine("acctNo to delete: ");
            Account *a = findAccountByAcctNo(who);
            if (!a) { cout << "Not found.\n"; continue; }
            if (!confirm("Confirm delete? This will remove account and transactions.")) continue;
            // erase account
            accounts.erase(remove_if(accounts.begin(), accounts.end(), [&](const Account &x){ return x.acctNo == who; }), accounts.end());
            transactions.erase(remove_if(transactions.begin(), transactions.end(), [&](const Transaction &t){ return t.acctNo == who; }), transactions.end());
            monthlyData.erase(remove_if(monthlyData.begin(), monthlyData.end(), [&](const MonthlyRecord &m){ return m.acctNo == who; }), monthlyData.end());
            budgets.erase(remove_if(budgets.begin(), budgets.end(), [&](const BudgetRecord &b){ return b.acctNo == who; }), budgets.end());
            cout << "Deleted.\n";
        } else if (ch == 4) {
            string who = inputLine("username or acctNo: ");
            Account *a = findAccountByUsername(who); if (!a) a = findAccountByAcctNo(who);
            if (!a) { cout << "Not found.\n"; continue; }
            string np; while (true) { np = inputLine("Enter new 4-digit PIN: "); if (np.size()==4 && all_of(np.begin(), np.end(), ::isdigit)) break; cout << "Invalid.\n"; }
            a->pin = np; saveDatabase();
            cout << "PIN reset.\n";
        } else if (ch == 5) {
            exportTransactionsCSV(TX_CSV);
            exportMonthlyCSV(MONTH_CSV);
            pressEnter();
        } else if (ch == 6) {
            adminRankingMenu();
        } else cout << "Invalid.\n";
    }
}

// ---------------- Interactive user dashboard ----------------
void userDashboard(Account &acc) {
    while (true) {
        cout << "\n===== Account & Expense Manager =====\n\nWELCOME " << acc.firstName << "\n\n";
        cout << "1. Account Detail\n2. Balance\n3. Deposit\n4. Expenses\n5. History\n6. Add Monthly\n7. View Monthly\n8. Set Budgets\n9. View Budgets\n10. Export CSV\n11. Logout\n";
        int c = inputInt("\nChoose: ");
        if (c == 11) return;
        if (c == 1) printAccountDetail(acc);
        else if (c == 2) showBalance(acc);
        else if (c == 3) depositFlow(acc);
        else if (c == 4) expenseFlow(acc);
        else if (c == 5) viewHistoryMenu(acc);
        else if (c == 6) monthlyEntryFlow(acc);
        else if (c == 7) viewMonthlyReport(acc.acctNo);
        else if (c == 8) setBudgets(acc.acctNo);
        else if (c == 9) viewBudgetsUsage(acc.acctNo);
        else if (c == 10) {
            string txfile = "transactions_" + acc.acctNo + ".csv";
            string mfile = "monthly_" + acc.acctNo + ".csv";
            exportTransactionsCSV(txfile);
            exportMonthlyCSV(mfile);
        } else cout << "Invalid choice.\n";
        pressEnter();
    }
}

// ---------------- Login ----------------
void loginFlow() {
    string username = inputLine("\nUsername: ");
    Account *a = findAccountByUsername(username);
    if (!a) { cout << "Not found.\n"; return; }
    string pin = inputLine("Enter 4-digit PIN: ");
    if (pin != a->pin) { cout << "Wrong PIN.\n"; return; }
    Account session = *a;
    userDashboard(session);
    for (auto &acc : accounts) if (acc.acctNo == session.acctNo) { acc.balance = session.balance; break; }
    saveDatabase();
}

// ---------------- main ----------------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout << "==== Account & Expense Manager (main3) ====\n";
    while (true) {
        cout << "\n*** Main ***\n\n1. Login\n2. Create\n3. Admin\n4. Quit\n\n";
        int ch = inputInt("Choose: ");
        if (ch == 4) break;
        if (ch == 1) loginFlow();
        else if (ch == 2) createAccountFlow();
        else if (ch == 3) adminMenu();
        else cout << "Invalid.\n";
    }
    cout << "Bye!\n";
    return 0;
}
