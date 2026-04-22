#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
using namespace std;

struct UserRec { char username[32]; char password[64]; char name[64]; char mail[64]; int privilege; };
static const char *USERS_FILE = "users.dat";

struct SessionList { string *arr; int sz; int cap; SessionList(): arr(nullptr), sz(0), cap(0) {} ~SessionList(){ delete[] arr; } void ensure(int n){ if(cap>=n) return; int nc = cap? cap*2:8; while(nc<n) nc*=2; string *na = new string[nc]; for(int i=0;i<sz;i++) na[i]=arr[i]; delete[] arr; arr=na; cap=nc; } bool has(const string &u){ for(int i=0;i<sz;i++) if(arr[i]==u) return true; return false; } void add(const string &u){ if(has(u)) return; ensure(sz+1); arr[sz++]=u; } bool remove(const string &u){ for(int i=0;i<sz;i++){ if(arr[i]==u){ arr[i]=arr[sz-1]; sz--; return true; } } return false; } void clear(){ sz=0; } } sessions;

static streampos find_user_pos(fstream &fs, const string &uname, UserRec &out){ fs.clear(); fs.seekg(0, ios::beg); UserRec r; while(fs.read(reinterpret_cast<char*>(&r), sizeof(UserRec))){ if(uname==r.username){ out=r; return fs.tellg() - static_cast<std::streamoff>(sizeof(UserRec)); } } return -1; }
static bool load_user(const string &uname, UserRec &out){ fstream fs(USERS_FILE, ios::in|ios::binary); if(!fs) return false; streampos p = find_user_pos(fs, uname, out); return p!=-1; }
static long long users_count(){ ifstream in(USERS_FILE, ios::binary); if(!in) return 0; in.seekg(0, ios::end); streamoff sz = in.tellg(); return sz<0?0: (sz/sizeof(UserRec)); }
static bool append_user(const UserRec &u){ ofstream out(USERS_FILE, ios::app|ios::binary); if(!out) return false; out.write(reinterpret_cast<const char*>(&u), sizeof(UserRec)); return out.good(); }
static void memset_user(UserRec &u){ memset(&u, 0, sizeof(UserRec)); u.privilege=0; }

struct TrainRec {
    char trainID[32]; int stationNum; int seatNum; char type; int start_hr, start_mi; int sale_s_m, sale_s_d, sale_e_m, sale_e_d; bool released; bool deleted;
    char stations[100][32]; int prices[100]; int travel[100]; int stopover[100];
};
static const char *TRAINS_FILE = "trains.dat";

static streampos find_train_pos(fstream &fs, const string &tid, TrainRec &out){ fs.clear(); fs.seekg(0, ios::beg); TrainRec r; while(fs.read(reinterpret_cast<char*>(&r), sizeof(TrainRec))){ if(!r.deleted && tid==r.trainID){ out=r; return fs.tellg() - static_cast<std::streamoff>(sizeof(TrainRec)); } } return -1; }
static bool load_train(const string &tid, TrainRec &out){ fstream fs(TRAINS_FILE, ios::in|ios::binary); if(!fs) return false; streampos p = find_train_pos(fs, tid, out); return p!=-1; }
static bool write_train_at(streampos pos, const TrainRec &t){ fstream fs(TRAINS_FILE, ios::in|ios::out|ios::binary); if(!fs) return false; fs.seekp(pos, ios::beg); fs.write(reinterpret_cast<const char*>(&t), sizeof(TrainRec)); return fs.good(); }
static bool append_train(const TrainRec &t){ ofstream out(TRAINS_FILE, ios::app|ios::binary); if(!out) return false; out.write(reinterpret_cast<const char*>(&t), sizeof(TrainRec)); return out.good(); }

static bool parse_kv(const string &line, string &cmd, pair<char,string> kvs[], int &kvn){ kvn=0; cmd.clear(); istringstream iss(line); if(!(iss>>cmd)) return false; string tok; while(iss>>tok){ if(tok.size()==2 && tok[0]=='-' && tok[1]){ string val; if(!(iss>>val)) return false; kvs[kvn++] = {tok[1], val}; if(kvn>=64) break; } } return true; }
static bool is_logged_in(const string &u){ return sessions.has(u); }

static int md_to_ordinal(int m, int d){ int base=0; if(m==6) base=0; else if(m==7) base=30; else if(m==8) base=61; return base + d - 1; }
static void ordinal_to_md(int ord, int &m, int &d){ if(ord<30){ m=6; d=ord+1; } else if(ord<61){ m=7; d=ord-30+1; } else { m=8; d=ord-61+1; } }
static void add_minutes(int &m, int &d, int &hr, int &mi, int add){ int total = hr*60 + mi + add; hr = (total/60)%24; mi = total%60; int days = (total/60)/24; int ord = md_to_ordinal(m,d); ord += days; if(ord<0) ord=0; if(ord> (30+31+31-1)) ord = (30+31+31-1); ordinal_to_md(ord, m, d); }
static void parse_hm(const string &s, int &h, int &mi){ h=((s[0]-'0')*10 + (s[1]-'0')); mi=((s[3]-'0')*10 + (s[4]-'0')); }
static void parse_md(const string &s, int &m, int &d){ m=((s[0]-'0')*10 + (s[1]-'0')); d=((s[3]-'0')*10 + (s[4]-'0')); }
static void fmt_hm(string &out, int h, int mi){ char buf[6]; snprintf(buf, sizeof(buf), "%02d:%02d", h, mi); out=buf; }
static void fmt_md(string &out, int m, int d){ char buf[6]; snprintf(buf, sizeof(buf), "%02d-%02d", m, d); out=buf; }

static int split_pipe(const string &s, string out[], int maxn){ int n=0; int i=0; while(i<(int)s.size() && n<maxn){ int j=i; while(j<(int)s.size() && s[j]!='|') j++; out[n++] = s.substr(i, j-i); i = j+1; } return n; }
static int to_int(const string &s){ return atoi(s.c_str()); }

int main(){ ios::sync_with_stdio(false); cin.tie(nullptr);
    string line;
    while (getline(cin, line)){
        if(line.empty()) continue;
        string cmd; pair<char,string> kvs[64]; int kvn=0; if(!parse_kv(line, cmd, kvs, kvn)){ cout<<-1<<"\n"; continue; }
        if(cmd=="exit") { sessions.clear(); cout<<"bye\n"; break; }
        else if(cmd=="clean") { sessions.clear(); remove(USERS_FILE); remove(TRAINS_FILE); cout<<0<<"\n"; }
        else if(cmd=="add_user"){
            string c,u,p,n,m; int g=0; bool has_g=false; for(int i=0;i<kvn;i++){ char k=kvs[i].first; const string &v=kvs[i].second; if(k=='c') c=v; else if(k=='u') u=v; else if(k=='p') p=v; else if(k=='n') n=v; else if(k=='m') m=v; else if(k=='g'){ has_g=true; g=to_int(v); } }
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
            string c,u,p,n,m; bool set_p=false,set_n=false,set_m=false,set_g=false; int g=0; for(int i=0;i<kvn;i++){ char k=kvs[i].first; const string &v=kvs[i].second; if(k=='c') c=v; else if(k=='u') u=v; else if(k=='p'){ p=v; set_p=true; } else if(k=='n'){ n=v; set_n=true; } else if(k=='m'){ m=v; set_m=true; } else if(k=='g'){ g=to_int(v); set_g=true; } }
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
        else if(cmd=="add_train"){
            TrainRec tr; memset(&tr,0,sizeof(tr)); tr.deleted=false; tr.released=false;
            string tid, s_stations, s_prices, s_travel, s_stop, s_sale; int n=0, seat=0; char type='G'; string xhm;
            for(int i=0;i<kvn;i++){ char k=kvs[i].first; const string &v=kvs[i].second; if(k=='i') tid=v; else if(k=='n') n=to_int(v); else if(k=='m') seat=to_int(v); else if(k=='s') s_stations=v; else if(k=='p') s_prices=v; else if(k=='x') xhm=v; else if(k=='t') s_travel=v; else if(k=='o') s_stop=v; else if(k=='d') s_sale=v; else if(k=='y') type=v.empty()? 'G': v[0]; }
            if(tid.empty()||n<2||n>100||seat<=0||s_stations.empty()||s_prices.empty()||xhm.empty()||s_travel.empty()||s_sale.empty()){ cout<<-1<<"\n"; continue; }
            TrainRec exist; if(load_train(tid, exist)){ cout<<-1<<"\n"; continue; }
            tr.stationNum=n; tr.seatNum=seat; tr.type=type; strncpy(tr.trainID, tid.c_str(), sizeof(tr.trainID)-1);
            parse_hm(xhm, tr.start_hr, tr.start_mi);
            string parts[128]; int cnt;
            cnt = split_pipe(s_stations, parts, 100); if(cnt!=n){ cout<<-1<<"\n"; continue; } for(int i=0;i<n;i++){ memset(tr.stations[i],0,sizeof(tr.stations[i])); strncpy(tr.stations[i], parts[i].c_str(), sizeof(tr.stations[i])-1); }
            cnt = split_pipe(s_prices, parts, 100); if(cnt!=n-1){ cout<<-1<<"\n"; continue; } for(int i=0;i<n-1;i++) tr.prices[i]=to_int(parts[i]);
            cnt = split_pipe(s_travel, parts, 100); if(cnt!=n-1){ cout<<-1<<"\n"; continue; } for(int i=0;i<n-1;i++) tr.travel[i]=to_int(parts[i]);
            if(n==2){ if(s_stop!="_") { cout<<-1<<"\n"; continue; } tr.stopover[0]=0; }
            else { cnt = split_pipe(s_stop, parts, 100); if(cnt!=n-2){ cout<<-1<<"\n"; continue; } for(int i=0;i<n-2;i++) tr.stopover[i]=to_int(parts[i]); }
            string sd[2]; cnt = split_pipe(s_sale, sd, 2); if(cnt!=2){ cout<<-1<<"\n"; continue; } parse_md(sd[0], tr.sale_s_m, tr.sale_s_d); parse_md(sd[1], tr.sale_e_m, tr.sale_e_d);
            if(!append_train(tr)) { cout<<-1<<"\n"; } else { cout<<0<<"\n"; }
        }
        else if(cmd=="release_train"){
            string tid; for(int i=0;i<kvn;i++) if(kvs[i].first=='i') tid=kvs[i].second;
            TrainRec tr; fstream fs(TRAINS_FILE, ios::in|ios::out|ios::binary); if(!fs){ cout<<-1<<"\n"; continue; }
            streampos pos = find_train_pos(fs, tid, tr); if(pos==-1 || tr.deleted){ cout<<-1<<"\n"; continue; }
            if(tr.released){ cout<<-1<<"\n"; continue; }
            tr.released=true; fs.seekp(pos, ios::beg); fs.write(reinterpret_cast<const char*>(&tr), sizeof(TrainRec)); if(!fs.good()){ cout<<-1<<"\n"; } else { cout<<0<<"\n"; }
        }
        else if(cmd=="query_train"){
            string tid, dmd; for(int i=0;i<kvn;i++){ if(kvs[i].first=='i') tid=kvs[i].second; else if(kvs[i].first=='d') dmd=kvs[i].second; }
            TrainRec tr; if(!load_train(tid, tr)){ cout<<-1<<"\n"; continue; }
            int qm, qd; parse_md(dmd, qm, qd); int s_ord = md_to_ordinal(tr.sale_s_m, tr.sale_s_d); int e_ord = md_to_ordinal(tr.sale_e_m, tr.sale_e_d); int q_ord = md_to_ordinal(qm, qd); if(q_ord<s_ord || q_ord>e_ord){ cout<<-1<<"\n"; continue; }
            cout<<tr.trainID<<" "<<tr.type<<"\n";
            int m=qm, d=qd, hr=tr.start_hr, mi=tr.start_mi; int cum_price=0;
            for(int i=0;i<tr.stationNum;i++){
                if(i==0){ string md, hm; fmt_md(md, m, d); fmt_hm(hm, hr, mi); cout<<tr.stations[i]<<" xx-xx xx:xx -> "<<md<<" "<<hm<<" "<<cum_price<<" "<<tr.seatNum<<"\n"; if(tr.stationNum>1){ add_minutes(m,d,hr,mi, tr.travel[0]); } }
                else if(i==tr.stationNum-1){ string md, hm; fmt_md(md, m, d); fmt_hm(hm, hr, mi); cum_price += tr.prices[i-1]; cout<<tr.stations[i]<<" "<<md<<" "<<hm<<" -> xx-xx xx:xx "<<cum_price<<" x\n"; }
                else { string a_md, a_hm, l_md, l_hm; fmt_md(a_md, m, d); fmt_hm(a_hm, hr, mi); cum_price += tr.prices[i-1]; add_minutes(m,d,hr,mi, tr.stopover[i-1]); fmt_md(l_md, m, d); fmt_hm(l_hm, hr, mi); cout<<tr.stations[i]<<" "<<a_md<<" "<<a_hm<<" -> "<<l_md<<" "<<l_hm<<" "<<cum_price<<" "<<tr.seatNum<<"\n"; add_minutes(m,d,hr,mi, tr.travel[i]); }
            }
        }
        else if(cmd=="delete_train"){
            string tid; for(int i=0;i<kvn;i++) if(kvs[i].first=='i') tid=kvs[i].second; TrainRec tr; fstream fs(TRAINS_FILE, ios::in|ios::out|ios::binary); if(!fs){ cout<<-1<<"\n"; continue; } streampos pos = find_train_pos(fs, tid, tr); if(pos==-1){ cout<<-1<<"\n"; continue; } if(tr.released){ cout<<-1<<"\n"; continue; } tr.deleted=true; fs.seekp(pos, ios::beg); fs.write(reinterpret_cast<const char*>(&tr), sizeof(TrainRec)); if(!fs.good()){ cout<<-1<<"\n"; } else { cout<<0<<"\n"; }
        }
        else if(cmd=="query_ticket"){
            string S,T,D, pref="time"; for(int i=0;i<kvn;i++){ char k=kvs[i].first; const string &v=kvs[i].second; if(k=='s') S=v; else if(k=='t') T=v; else if(k=='d') D=v; else if(k=='p') pref=v; }
            int qm,qd; parse_md(D,qm,qd); int q_ord=md_to_ordinal(qm,qd);
            struct Item{ string tid, from, to; int l_m,l_d,l_hr,l_mi; int a_m,a_d,a_hr,a_mi; int price; int seat; int time_cost; };
            Item *items=nullptr; int it_sz=0,it_cap=0; auto ensure=[&](int n){ if(it_cap>=n) return; int nc= it_cap? it_cap*2:32; while(nc<n) nc*=2; Item *ni=new Item[nc]; for(int i=0;i<it_sz;i++) ni[i]=items[i]; delete[] items; items=ni; it_cap=nc; };
            fstream fs(TRAINS_FILE, ios::in|ios::binary); if(fs){ TrainRec tr; while(fs.read(reinterpret_cast<char*>(&tr), sizeof(TrainRec))){ if(tr.deleted || !tr.released) continue; int si=-1, ti=-1; for(int i=0;i<tr.stationNum;i++){ if(S==tr.stations[i]) si=i; if(T==tr.stations[i]) ti=i; }
                    if(si==-1 || ti==-1 || si>=ti) continue;
                    long long minutes_to_leave_s=0; for(int i=0;i<si;i++){ minutes_to_leave_s += tr.travel[i]; if(i>0) minutes_to_leave_s += tr.stopover[i-1]; }
                    if(si>0) minutes_to_leave_s += tr.stopover[si-1];
                    int day_off = (int)((tr.start_hr*60 + tr.start_mi + minutes_to_leave_s)/ (24*60));
                    int base_ord = q_ord - day_off; int s_ord = md_to_ordinal(tr.sale_s_m, tr.sale_s_d); int e_ord = md_to_ordinal(tr.sale_e_m, tr.sale_e_d); if(base_ord<s_ord || base_ord>e_ord) continue;
                    int m=tr.sale_s_m, d=tr.sale_s_d; ordinal_to_md(base_ord, m, d); int hr=tr.start_hr, mi=tr.start_mi; // start at base
                    // advance to leaving s
                    add_minutes(m,d,hr,mi, (int)minutes_to_leave_s);
                    int l_m=m,l_d=d,l_hr=hr,l_mi=mi;
                    // compute arrival at t
                    long long travel_between=0; int price=0; for(int i=si;i<ti;i++){ travel_between += tr.travel[i]; price += tr.prices[i]; if(i<ti-1) travel_between += tr.stopover[i]; }
                    add_minutes(m,d,hr,mi, (int)travel_between);
                    int a_m=m,a_d=d,a_hr=hr,a_mi=mi;
                    int time_cost = (int)travel_between;
                    ensure(it_sz+1); items[it_sz++] = Item{ string(tr.trainID), string(tr.stations[si]), string(tr.stations[ti]), l_m,l_d,l_hr,l_mi, a_m,a_d,a_hr,a_mi, price, tr.seatNum, time_cost };
                }
            }
            // sort
            auto less_by = [&](const Item &A, const Item &B){ if(pref=="time"){ if(A.time_cost!=B.time_cost) return A.time_cost<B.time_cost; } else { if(A.price!=B.price) return A.price<B.price; } return A.tid < B.tid; };
            for(int i=1;i<it_sz;i++){ Item key=items[i]; int j=i-1; while(j>=0 && less_by(key, items[j])){ items[j+1]=items[j]; j--; } items[j+1]=key; }
            cout<<it_sz<<"\n";
            for(int i=0;i<it_sz;i++){
                string lmd, lhm, amd, ahm; fmt_md(lmd, items[i].l_m, items[i].l_d); fmt_hm(lhm, items[i].l_hr, items[i].l_mi); fmt_md(amd, items[i].a_m, items[i].a_d); fmt_hm(ahm, items[i].a_hr, items[i].a_mi);
                cout<<items[i].tid<<" "<<items[i].from<<" "<<lmd<<" "<<lhm<<" -> "<<items[i].to<<" "<<amd<<" "<<ahm<<" "<<items[i].price<<" "<<items[i].seat<<"\n";
            }
            delete[] items;
        }
        else { cout<<-1<<"\n"; }
    }
    return 0;
}
