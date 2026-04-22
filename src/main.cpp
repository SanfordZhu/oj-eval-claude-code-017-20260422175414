#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
using namespace std;

struct UserRec {
    char username[32];
    char password[64];
    char name[64];
    char mail[64];
    int privilege;
};

static const char *USERS_FILE = "users.dat";

// Simple dynamic array for logged-in usernames
struct SessionList {
    string *arr;
    int sz;
    int cap;
    SessionList(): arr(nullptr), sz(0), cap(0) {}
    ~SessionList(){ delete[] arr; }
    void ensure(int n){ if(cap>=n) return; int nc = cap? cap*2:8; while(nc<n) nc*=2; string *na = new string[nc]; for(int i=0;i<sz;i++) na[i]=arr[i]; delete[] arr; arr=na; cap=nc; }
    bool has(const string &u){ for(int i=0;i<sz;i++) if(arr[i]==u) return true; return false; }
    void add(const string &u){ if(has(u)) return; ensure(sz+1); arr[sz++]=u; }
    bool remove(const string &u){ for(int i=0;i<sz;i++){ if(arr[i]==u){ arr[i]=arr[sz-1]; sz--; return true; } } return false; }
    void clear(){ sz=0; }
} sessions;

static streampos find_user_pos(fstream &fs, const string &uname, UserRec &out){
    fs.clear(); fs.seekg(0, ios::beg); UserRec r; while(fs.read(reinterpret_cast<char*>(&r), sizeof(UserRec))){ if(uname==r.username){ out=r; return fs.tellg() - static_cast<std::streamoff>(sizeof(UserRec)); } }
    return -1;
}

static bool load_user(const string &uname, UserRec &out){ fstream fs(USERS_FILE, ios::in|ios::binary); if(!fs) return false; streampos p = find_user_pos(fs, uname, out); return p!=-1; }

static long long users_count(){ ifstream in(USERS_FILE, ios::binary); if(!in) return 0; in.seekg(0, ios::end); streamoff sz = in.tellg(); return sz<0?0: (sz/sizeof(UserRec)); }

static bool write_user_at(streampos pos, const UserRec &u){ fstream fs(USERS_FILE, ios::in|ios::out|ios::binary); if(!fs) return false; fs.seekp(pos, ios::beg); fs.write(reinterpret_cast<const char*>(&u), sizeof(UserRec)); return fs.good(); }

static bool append_user(const UserRec &u){ ofstream out(USERS_FILE, ios::app|ios::binary); if(!out) return false; out.write(reinterpret_cast<const char*>(&u), sizeof(UserRec)); return out.good(); }

static void memset_user(UserRec &u){ memset(&u, 0, sizeof(UserRec)); u.privilege=0; }

static bool parse_kv(const string &line, string &cmd, pair<char,string> kvs[], int &kvn){
    kvn=0; cmd.clear(); istringstream iss(line); if(!(iss>>cmd)) return false; string tok; while(iss>>tok){ if(tok.size()==2 && tok[0]=='-' && tok[1]){ string val; if(!(iss>>val)) return false; kvs[kvn++] = {tok[1], val}; if(kvn>=32) break; } else { /* ignore */ } }
    return true;
}

static bool is_logged_in(const string &u){ return sessions.has(u); }

int main(){ ios::sync_with_stdio(false); cin.tie(nullptr);
    string line;
    while (getline(cin, line)){
        if(line.empty()) continue;
        string cmd; pair<char,string> kvs[32]; int kvn=0; if(!parse_kv(line, cmd, kvs, kvn)){ cout<<-1<<"\n"; continue; }
        if(cmd=="exit") { sessions.clear(); cout<<"bye\n"; break; }
        else if(cmd=="clean") { sessions.clear(); remove(USERS_FILE); cout<<0<<"\n"; }
        else if(cmd=="add_user"){
            string c,u,p,n,m; int g=0; bool has_g=false; for(int i=0;i<kvn;i++){ char k=kvs[i].first; const string &v=kvs[i].second; if(k=='c') c=v; else if(k=='u') u=v; else if(k=='p') p=v; else if(k=='n') n=v; else if(k=='m') m=v; else if(k=='g'){ has_g=true; g=stoi(v); } }
            if(users_count()==0){ if(u.empty()||p.empty()||n.empty()||m.empty()){ cout<<-1<<"\n"; continue; } UserRec exist; if(load_user(u, exist)){ cout<<-1<<"\n"; continue; } UserRec nu; memset_user(nu); strncpy(nu.username,u.c_str(),sizeof(nu.username)-1); strncpy(nu.password,p.c_str(),sizeof(nu.password)-1); strncpy(nu.name,n.c_str(),sizeof(nu.name)-1); strncpy(nu.mail,m.c_str(),sizeof(nu.mail)-1); nu.privilege=10; if(append_user(nu)) cout<<0<<"\n"; else cout<<-1<<"\n"; continue; }
            if(!is_logged_in(c)){ cout<<-1<<"\n"; continue; }
            UserRec cu; if(!load_user(c, cu)){ cout<<-1<<"\n"; continue; }
            if(!has_g || g>=cu.privilege){ cout<<-1<<"\n"; continue; }
            if(u.empty()||p.empty()||n.empty()||m.empty()){ cout<<-1<<"\n"; continue; }
            UserRec exist; if(load_user(u, exist)){ cout<<-1<<"\n"; continue; }
            UserRec nu; memset_user(nu); strncpy(nu.username,u.c_str(),sizeof(nu.username)-1); strncpy(nu.password,p.c_str(),sizeof(nu.password)-1); strncpy(nu.name,n.c_str(),sizeof(nu.name)-1); strncpy(nu.mail,m.c_str(),sizeof(nu.mail)-1); nu.privilege=g; if(append_user(nu)) cout<<0<<"\n"; else cout<<-1<<"\n";
        }
        else if(cmd=="login"){
            string u,p; for(int i=0;i<kvn;i++){ if(kvs[i].first=='u') u=kvs[i].second; else if(kvs[i].first=='p') p=kvs[i].second; }
            if(u.empty()||p.empty()){ cout<<-1<<"\n"; continue; }
            if(sessions.has(u)){ cout<<-1<<"\n"; continue; }
            UserRec ur; if(!load_user(u, ur)){ cout<<-1<<"\n"; continue; }
            if(p!=string(ur.password)){ cout<<-1<<"\n"; continue; }
            sessions.add(u); cout<<0<<"\n";
        }
        else if(cmd=="logout"){
            string u; for(int i=0;i<kvn;i++){ if(kvs[i].first=='u') u=kvs[i].second; }
            if(u.empty()){ cout<<-1<<"\n"; continue; }
            if(!sessions.remove(u)){ cout<<-1<<"\n"; } else { cout<<0<<"\n"; }
        }
        else if(cmd=="query_profile"){
            string c,u; for(int i=0;i<kvn;i++){ if(kvs[i].first=='c') c=kvs[i].second; else if(kvs[i].first=='u') u=kvs[i].second; }
            if(!is_logged_in(c)){ cout<<-1<<"\n"; continue; }
            UserRec cu, uu; if(!load_user(c, cu) || !load_user(u, uu)){ cout<<-1<<"\n"; continue; }
            if(!(cu.privilege>uu.privilege || c==u)){ cout<<-1<<"\n"; continue; }
            cout<<uu.username<<" "<<uu.name<<" "<<uu.mail<<" "<<uu.privilege<<"\n";
        }
        else if(cmd=="modify_profile"){
            string c,u,p,n,m; bool set_p=false,set_n=false,set_m=false,set_g=false; int g=0; for(int i=0;i<kvn;i++){ char k=kvs[i].first; const string &v=kvs[i].second; if(k=='c') c=v; else if(k=='u') u=v; else if(k=='p'){ p=v; set_p=true; } else if(k=='n'){ n=v; set_n=true; } else if(k=='m'){ m=v; set_m=true; } else if(k=='g'){ g=stoi(v); set_g=true; } }
            if(!is_logged_in(c)){ cout<<-1<<"\n"; continue; }
            UserRec cu, uu; if(!load_user(c, cu) || !load_user(u, uu)){ cout<<-1<<"\n"; continue; }
            if(!(cu.privilege>uu.privilege || c==u)){ cout<<-1<<"\n"; continue; }
            if(set_g && !(g<cu.privilege)){ cout<<-1<<"\n"; continue; }
            if(set_p) { memset(uu.password,0,sizeof(uu.password)); strncpy(uu.password,p.c_str(),sizeof(uu.password)-1); }
            if(set_n) { memset(uu.name,0,sizeof(uu.name)); strncpy(uu.name,n.c_str(),sizeof(uu.name)-1); }
            if(set_m) { memset(uu.mail,0,sizeof(uu.mail)); strncpy(uu.mail,m.c_str(),sizeof(uu.mail)-1); }
            if(set_g) uu.privilege=g;
            fstream fs(USERS_FILE, ios::in|ios::out|ios::binary); if(!fs){ cout<<-1<<"\n"; continue; }
            streampos pos = find_user_pos(fs, u, cu); if(pos==-1){ cout<<-1<<"\n"; continue; }
            fs.seekp(pos, ios::beg); fs.write(reinterpret_cast<const char*>(&uu), sizeof(UserRec)); if(!fs.good()){ cout<<-1<<"\n"; continue; }
            cout<<uu.username<<" "<<uu.name<<" "<<uu.mail<<" "<<uu.privilege<<"\n";
        }
        else { cout<<-1<<"\n"; }
    }
    return 0;
}
