#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Money Manager: a single-file, console-based personal finance application.
// Data is stored locally in money_manager_data.txt.

struct Account {
    string name;
    double openingBalance = 0.0;
};

struct Transaction {
    int id = 0;
    string type;       // INCOME, EXPENSE, TRANSFER
    double amount = 0.0;
    string category;
    string date;       // YYYY-MM-DD
    string account;    // source/account for income, expense and transfer
    string toAccount;  // destination for transfer
    string note;
};

struct Budget {
    string month;      // YYYY-MM
    string category;
    double limit = 0.0;
};

struct Shortcut {
    string name;
    string type;       // INCOME or EXPENSE
    double amount = 0.0;
    string category;
    string account;
    string note;
};

class MoneyManager {
private:
    vector<Account> accounts;
    vector<Transaction> transactions;
    vector<Budget> budgets;
    vector<Shortcut> shortcuts;
    int nextId = 1;
    const string dataFile = "money_manager_data.txt";

    static string upper(string s) {
        transform(s.begin(), s.end(), s.begin(),
                  [](unsigned char c) { return static_cast<char>(toupper(c)); });
        return s;
    }

    static string trim(string s) {
        size_t first = s.find_first_not_of(" \t\r\n");
        if (first == string::npos) return "";
        size_t last = s.find_last_not_of(" \t\r\n");
        return s.substr(first, last - first + 1);
    }

    static bool validDate(const string& d) {
        if (d.size() != 10 || d[4] != '-' || d[7] != '-') return false;
        for (size_t i = 0; i < d.size(); ++i) {
            if (i == 4 || i == 7) continue;
            if (!isdigit(static_cast<unsigned char>(d[i]))) return false;
        }
        int month = stoi(d.substr(5, 2));
        int day = stoi(d.substr(8, 2));
        return month >= 1 && month <= 12 && day >= 1 && day <= 31;
    }

    static bool validMonth(const string& m) {
        if (m.size() != 7 || m[4] != '-') return false;
        for (size_t i = 0; i < m.size(); ++i) {
            if (i == 4) continue;
            if (!isdigit(static_cast<unsigned char>(m[i]))) return false;
        }
        int month = stoi(m.substr(5, 2));
        return month >= 1 && month <= 12;
    }

    int accountIndex(const string& name) const {
        for (size_t i = 0; i < accounts.size(); ++i)
            if (accounts[i].name == name) return static_cast<int>(i);
        return -1;
    }

    bool categoryExistsForBudget(const string& category) const {
        for (const auto& b : budgets)
            if (b.category == category) return true;
        return false;
    }

    double accountBalance(const string& accountName) const {
        int idx = accountIndex(accountName);
        if (idx < 0) return 0.0;
        double balance = accounts[idx].openingBalance;
        for (const auto& t : transactions) {
            if (t.type == "INCOME" && t.account == accountName) balance += t.amount;
            else if (t.type == "EXPENSE" && t.account == accountName) balance -= t.amount;
            else if (t.type == "TRANSFER") {
                if (t.account == accountName) balance -= t.amount;
                if (t.toAccount == accountName) balance += t.amount;
            }
        }
        return balance;
    }

    double totalBalance() const {
        double total = 0.0;
        for (const auto& a : accounts) total += accountBalance(a.name);
        return total;
    }

    double categoryExpense(const string& category, const string& month) const {
        double total = 0.0;
        for (const auto& t : transactions)
            if (t.type == "EXPENSE" && t.category == category &&
                t.date.size() >= 7 && t.date.substr(0, 7) == month)
                total += t.amount;
        return total;
    }

    void pause() const {
        cout << "\nPress Enter to continue...";
        string dummy;
        getline(cin, dummy);
    }

    int readInt(const string& prompt) const {
        while (true) {
            cout << prompt;
            string line;
            getline(cin, line);
            stringstream ss(line);
            int value;
            char extra;
            if (ss >> value && !(ss >> extra)) return value;
            cout << "Please enter a valid whole number.\n";
        }
    }

    double readAmount(const string& prompt) const {
        while (true) {
            cout << prompt;
            string line;
            getline(cin, line);
            stringstream ss(line);
            double value;
            char extra;
            if ((ss >> value) && !(ss >> extra) && value > 0.0) return value;
            cout << "Please enter a valid amount greater than zero.\n";
        }
    }

    string readNonEmpty(const string& prompt) const {
        while (true) {
            cout << prompt;
            string value;
            getline(cin, value);
            value = trim(value);
            if (!value.empty()) return value;
            cout << "This field cannot be empty.\n";
        }
    }

    string readDate() const {
        while (true) {
            string d = readNonEmpty("Date (YYYY-MM-DD): ");
            if (validDate(d)) return d;
            cout << "Use the format YYYY-MM-DD, for example 2026-11-19.\n";
        }
    }

    string chooseAccount(const string& prompt) const {
        if (accounts.empty()) return "";
        cout << "\nAvailable accounts:\n";
        for (size_t i = 0; i < accounts.size(); ++i)
            cout << "  " << i + 1 << ". " << accounts[i].name
                 << " (Balance: Rs. " << fixed << setprecision(2)
                 << accountBalance(accounts[i].name) << ")\n";
        while (true) {
            int n = readInt(prompt);
            if (n >= 1 && n <= static_cast<int>(accounts.size()))
                return accounts[n - 1].name;
            cout << "Choose a number from the list.\n";
        }
    }

    void printTransaction(const Transaction& t) const {
        cout << "ID: " << t.id << " | " << t.date << " | ";
        if (t.type == "TRANSFER") {
            cout << "TRANSFER | Rs. " << fixed << setprecision(2) << t.amount
                 << " | " << t.account << " -> " << t.toAccount;
        } else {
            cout << t.type << " | Rs. " << fixed << setprecision(2) << t.amount
                 << " | " << t.category << " | Account: " << t.account;
        }
        if (!t.note.empty()) cout << " | Note: " << t.note;
        cout << '\n';
    }

    void save() const {
        ofstream out(dataFile);
        if (!out) throw runtime_error("Could not open the data file for writing.");
        out << "MONEY_MANAGER_V1\n";
        for (const auto& a : accounts)
            out << "A " << quoted(a.name) << ' ' << setprecision(17) << a.openingBalance << '\n';
        for (const auto& t : transactions)
            out << "T " << t.id << ' ' << t.type << ' ' << setprecision(17) << t.amount << ' '
                << quoted(t.category) << ' ' << quoted(t.date) << ' '
                << quoted(t.account) << ' ' << quoted(t.toAccount) << ' ' << quoted(t.note) << '\n';
        for (const auto& b : budgets)
            out << "B " << quoted(b.month) << ' ' << quoted(b.category) << ' '
                << setprecision(17) << b.limit << '\n';
        for (const auto& s : shortcuts)
            out << "S " << quoted(s.name) << ' ' << s.type << ' ' << setprecision(17)
                << s.amount << ' ' << quoted(s.category) << ' '
                << quoted(s.account) << ' ' << quoted(s.note) << '\n';
        if (!out) throw runtime_error("An error occurred while saving your data.");
    }

    void load() {
        ifstream in(dataFile);
        if (!in) return; // First run: no data file yet.
        string line;
        getline(in, line); // Header
        while (getline(in, line)) {
            if (trim(line).empty()) continue;
            istringstream row(line);
            char kind;
            row >> kind;
            if (kind == 'A') {
                Account a;
                if (row >> quoted(a.name) >> a.openingBalance) accounts.push_back(a);
            } else if (kind == 'T') {
                Transaction t;
                if (row >> t.id >> t.type >> t.amount >> quoted(t.category)
                        >> quoted(t.date) >> quoted(t.account)
                        >> quoted(t.toAccount) >> quoted(t.note)) {
                    if ((t.type == "INCOME" || t.type == "EXPENSE" || t.type == "TRANSFER")
                        && t.amount > 0) {
                        transactions.push_back(t);
                        nextId = max(nextId, t.id + 1);
                    }
                }
            } else if (kind == 'B') {
                Budget b;
                if (row >> quoted(b.month) >> quoted(b.category) >> b.limit)
                    budgets.push_back(b);
            } else if (kind == 'S') {
                Shortcut s;
                if (row >> quoted(s.name) >> s.type >> s.amount
                        >> quoted(s.category) >> quoted(s.account) >> quoted(s.note))
                    shortcuts.push_back(s);
            }
        }
    }

    void addAccount() {
        string name = readNonEmpty("New account name (e.g. Cash, SBI, HDFC): ");
        if (accountIndex(name) >= 0) {
            cout << "An account with that name already exists.\n";
            return;
        }
        cout << "Opening balance (enter 0 if new): ";
        string raw;
        getline(cin, raw);
        double opening = 0.0;
        if (!raw.empty()) {
            stringstream ss(raw);
            char extra;
            if (!(ss >> opening) || (ss >> extra) || opening < 0) {
                cout << "Invalid opening balance. Account not created.\n";
                return;
            }
        }
        accounts.push_back({name, opening});
        save();
        cout << "Account created.\n";
    }

    void listAccounts() const {
        cout << "\n========== ACCOUNTS ==========" << '\n';
        if (accounts.empty()) { cout << "No accounts created yet.\n"; return; }
        for (const auto& a : accounts)
            cout << a.name << " | Opening: Rs. " << fixed << setprecision(2)
                 << a.openingBalance << " | Current: Rs. " << accountBalance(a.name) << '\n';
        cout << "Total assets: Rs. " << totalBalance() << '\n';
    }

    void addTransaction(const string& type) {
        if (accounts.empty()) {
            cout << "Create an account first using Account Management.\n";
            return;
        }
        Transaction t;
        t.id = nextId++;
        t.type = type;
        t.amount = readAmount("Amount (Rs.): ");
        if (type == "TRANSFER") {
            t.account = chooseAccount("Transfer FROM account number: ");
            t.toAccount = chooseAccount("Transfer TO account number: ");
            if (t.account == t.toAccount) {
                cout << "Source and destination accounts must be different.\n";
                --nextId;
                return;
            }
            t.category = "Transfer";
        } else {
            t.category = readNonEmpty("Category (Food, Travel, Bills, Salary, etc.): ");
            t.account = chooseAccount("Account number: ");
        }
        t.date = readDate();
        cout << "Note (optional, press Enter to skip): ";
        getline(cin, t.note);
        transactions.push_back(t);
        save();
        cout << "Transaction saved successfully. ID: " << t.id << '\n';
    }

    void showTransactions() const {
        cout << "\n========== TRANSACTION HISTORY ==========" << '\n';
        if (transactions.empty()) { cout << "No transactions recorded.\n"; return; }
        vector<Transaction> sorted = transactions;
        sort(sorted.begin(), sorted.end(), [](const Transaction& a, const Transaction& b) {
            if (a.date != b.date) return a.date > b.date;
            return a.id > b.id;
        });
        for (const auto& t : sorted) printTransaction(t);
    }

    void searchTransactions() const {
        string key = upper(readNonEmpty("Search category, account, note or date: "));
        bool found = false;
        for (const auto& t : transactions) {
            string hay = upper(t.category + " " + t.account + " " + t.toAccount + " " + t.note + " " + t.date + " " + t.type);
            if (hay.find(key) != string::npos) { printTransaction(t); found = true; }
        }
        if (!found) cout << "No matching transactions found.\n";
    }

    void deleteTransaction() {
        if (transactions.empty()) { cout << "No transactions to delete.\n"; return; }
        showTransactions();
        int id = readInt("Enter the transaction ID to delete: ");
        auto it = find_if(transactions.begin(), transactions.end(),
                          [id](const Transaction& t) { return t.id == id; });
        if (it == transactions.end()) { cout << "Transaction ID not found.\n"; return; }
        cout << "Delete this transaction? (y/n): ";
        string answer; getline(cin, answer);
        if (upper(answer) != "Y") { cout << "Deletion cancelled.\n"; return; }
        transactions.erase(it);
        save();
        cout << "Transaction deleted and data saved.\n";
    }

    void editTransaction() {
        if (transactions.empty()) { cout << "No transactions to edit.\n"; return; }
        showTransactions();
        int id = readInt("Enter transaction ID to edit: ");
        auto it = find_if(transactions.begin(), transactions.end(),
                          [id](const Transaction& t) { return t.id == id; });
        if (it == transactions.end()) { cout << "Transaction ID not found.\n"; return; }
        if (it->type == "TRANSFER") {
            cout << "Editing transfers is not supported in this version. Delete and re-enter it instead.\n";
            return;
        }
        cout << "Current amount: Rs. " << it->amount << "\n";
        it->amount = readAmount("New amount (Rs.): ");
        it->category = readNonEmpty("New category/source: ");
        it->date = readDate();
        cout << "New note (press Enter to clear): ";
        getline(cin, it->note);
        cout << "Choose the account for this transaction:\n";
        it->account = chooseAccount("Account number: ");
        save();
        cout << "Transaction updated and saved.\n";
    }

    void setBudget() {
        string month;
        while (true) {
            month = readNonEmpty("Budget month (YYYY-MM): ");
            if (validMonth(month)) break;
            cout << "Use YYYY-MM, for example 2026-11.\n";
        }
        string category = readNonEmpty("Budget category: ");
        double limit = readAmount("Budget limit (Rs.): ");
        auto it = find_if(budgets.begin(), budgets.end(), [&](const Budget& b) {
            return b.month == month && b.category == category;
        });
        if (it == budgets.end()) budgets.push_back({month, category, limit});
        else it->limit = limit;
        save();
        cout << "Budget saved.\n";
    }

    void budgetReport() const {
        string month;
        while (true) {
            month = readNonEmpty("Report month (YYYY-MM): ");
            if (validMonth(month)) break;
            cout << "Use YYYY-MM, for example 2026-11.\n";
        }
        cout << "\n========== BUDGET REPORT: " << month << " ==========" << '\n';
        bool found = false;
        for (const auto& b : budgets) {
            if (b.month != month) continue;
            found = true;
            double spent = categoryExpense(b.category, month);
            double remaining = b.limit - spent;
            cout << b.category << " | Budget: Rs. " << fixed << setprecision(2) << b.limit
                 << " | Spent: Rs. " << spent << " | ";
            if (remaining >= 0) cout << "Remaining: Rs. " << remaining;
            else cout << "Over budget by: Rs. " << -remaining;
            cout << '\n';
        }
        if (!found) cout << "No budgets set for this month.\n";
    }

    void monthlyReport() const {
        string month;
        while (true) {
            month = readNonEmpty("Month to analyse (YYYY-MM): ");
            if (validMonth(month)) break;
            cout << "Use YYYY-MM, for example 2026-11.\n";
        }
        double income = 0, expense = 0;
        map<string, double> byCategory;
        for (const auto& t : transactions) {
            if (t.date.size() < 7 || t.date.substr(0, 7) != month) continue;
            if (t.type == "INCOME") income += t.amount;
            else if (t.type == "EXPENSE") { expense += t.amount; byCategory[t.category] += t.amount; }
        }
        cout << "\n========== MONTHLY REPORT: " << month << " ==========" << '\n';
        cout << fixed << setprecision(2)
             << "Income: Rs. " << income << "\nExpenses: Rs. " << expense
             << "\nNet cash flow: Rs. " << income - expense << '\n';
        cout << "\nExpense by category:\n";
        if (byCategory.empty()) cout << "No expenses recorded for this month.\n";
        for (const auto& item : byCategory)
            cout << "  " << item.first << ": Rs. " << item.second << '\n';
        budgetReportForMonth(month);
    }

    void budgetReportForMonth(const string& month) const {
        bool any = false;
        cout << "\nBudget status:\n";
        for (const auto& b : budgets) {
            if (b.month != month) continue;
            any = true;
            double spent = categoryExpense(b.category, month);
            cout << "  " << b.category << ": Rs. " << spent << " / Rs. " << b.limit;
            if (spent > b.limit) cout << " (OVER BUDGET)";
            cout << '\n';
        }
        if (!any) cout << "  No budgets set for this month.\n";
    }

    void categoryStatistics() const {
        map<string, double> totals;
        double all = 0;
        for (const auto& t : transactions) {
            if (t.type == "EXPENSE") { totals[t.category] += t.amount; all += t.amount; }
        }
        cout << "\n========== ALL-TIME CATEGORY STATISTICS ==========" << '\n';
        if (totals.empty()) { cout << "No expense data available.\n"; return; }
        for (const auto& item : totals) {
            double pct = all > 0 ? item.second * 100.0 / all : 0;
            cout << left << setw(20) << item.first << right << " Rs. "
                 << fixed << setprecision(2) << item.second << " (" << setprecision(1)
                 << pct << "%)\n";
        }
        cout << "Total expenses: Rs. " << fixed << setprecision(2) << all << '\n';
    }

    void addShortcut() {
        if (accounts.empty()) { cout << "Create an account first.\n"; return; }
        Shortcut s;
        s.name = readNonEmpty("Bookmark name (e.g. Daily Lunch): ");
        cout << "1. Expense\n2. Income\n";
        int type = readInt("Choose type: ");
        if (type != 1 && type != 2) { cout << "Invalid type.\n"; return; }
        s.type = type == 1 ? "EXPENSE" : "INCOME";
        s.amount = readAmount("Default amount (Rs.): ");
        s.category = readNonEmpty("Category/source: ");
        s.account = chooseAccount("Account number: ");
        cout << "Default note (optional): "; getline(cin, s.note);
        shortcuts.push_back(s);
        save();
        cout << "Bookmark saved.\n";
    }

    void useShortcut() {
        if (shortcuts.empty()) { cout << "No bookmarks saved yet.\n"; return; }
        cout << "\n========== QUICK ADD BOOKMARKS ==========" << '\n';
        for (size_t i = 0; i < shortcuts.size(); ++i)
            cout << i + 1 << ". " << shortcuts[i].name << " | " << shortcuts[i].type
                 << " | Rs. " << fixed << setprecision(2) << shortcuts[i].amount
                 << " | " << shortcuts[i].category << '\n';
        int n = readInt("Choose bookmark number (0 to cancel): ");
        if (n == 0) return;
        if (n < 1 || n > static_cast<int>(shortcuts.size())) { cout << "Invalid selection.\n"; return; }
        const Shortcut& s = shortcuts[n - 1];
        Transaction t;
        t.id = nextId++;
        t.type = s.type;
        t.amount = s.amount;
        t.category = s.category;
        t.account = s.account;
        t.note = s.note;
        t.date = readDate();
        transactions.push_back(t);
        save();
        cout << "Quick transaction saved. ID: " << t.id << '\n';
    }

    void exportCsv() const {
        ofstream out("money_manager_export.csv");
        if (!out) { cout << "Could not create CSV file.\n"; return; }
        out << "ID,Type,Amount,Category,Date,Account,To Account,Note\n";
        for (const auto& t : transactions) {
            auto clean = [](string s) {
                replace(s.begin(), s.end(), ',', ';');
                replace(s.begin(), s.end(), '\n', ' ');
                return s;
            };
            out << t.id << ',' << t.type << ',' << fixed << setprecision(2) << t.amount << ','
                << clean(t.category) << ',' << t.date << ',' << clean(t.account) << ','
                << clean(t.toAccount) << ',' << clean(t.note) << '\n';
        }
        cout << "CSV exported to money_manager_export.csv\n";
    }

public:
    MoneyManager() {
        load();
        if (accounts.empty()) accounts.push_back({"Cash", 0.0});
    }

    void run() {
        while (true) {
            cout << "\n==============================================\n";
            cout << "          PERSONAL MONEY MANAGER\n";
            cout << "==============================================\n";
            cout << "Total assets: Rs. " << fixed << setprecision(2) << totalBalance() << '\n';
            cout << "\nTRANSACTIONS\n"
                 << " 1. Add expense\n 2. Add income\n 3. Transfer between accounts\n"
                 << " 4. View transaction history\n 5. Edit transaction\n 6. Delete transaction\n"
                 << " 7. Search transactions\n\nACCOUNTS & PLANNING\n"
                 << " 8. View accounts and balances\n 9. Add account\n"
                 << "10. Set monthly budget\n11. View budget report\n"
                 << "12. Monthly report\n13. Category statistics\n\nQUICK TOOLS\n"
                 << "14. Create quick-add bookmark\n15. Use quick-add bookmark\n"
                 << "16. Export transactions to CSV\n17. Save and exit\n";
            int choice = readInt("\nChoose an option: ");
            try {
                switch (choice) {
                    case 1: addTransaction("EXPENSE"); break;
                    case 2: addTransaction("INCOME"); break;
                    case 3: addTransaction("TRANSFER"); break;
                    case 4: showTransactions(); break;
                    case 5: editTransaction(); break;
                    case 6: deleteTransaction(); break;
                    case 7: searchTransactions(); break;
                    case 8: listAccounts(); break;
                    case 9: addAccount(); break;
                    case 10: setBudget(); break;
                    case 11: budgetReport(); break;
                    case 12: monthlyReport(); break;
                    case 13: categoryStatistics(); break;
                    case 14: addShortcut(); break;
                    case 15: useShortcut(); break;
                    case 16: exportCsv(); break;
                    case 17: save(); cout << "\nData saved. Goodbye!\n"; return;
                    default: cout << "Please choose an option from 1 to 17.\n";
                }
            } catch (const exception& e) {
                cout << "\nError: " << e.what() << '\n';
            }
            pause();
        }
    }
};

int main() {
    try {
        MoneyManager app;
        app.run();
    } catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
