#ifndef _MAC_Check
#define _MAC_Check

/* Class for storing MAC Check data and doing the Check */

#include <vector>
#include <deque>
using namespace std;

#include "Protocols/Share.h"
#include "Networking/Player.h"
#include "Protocols/MAC_Check_Base.h"
#include "Tools/time-func.h"
#include "Tools/Coordinator.h"

/* The MAX number of things we will partially open before running
 * a MAC Check
 *
 * Keep this at much less than 1MB of data to be able to cope with
 * multi-threaded players
 *
 */
#define POPEN_MAX 1000000

/**
 * Sum and broadcast values via a tree of players
 */
template <class T>
class TreeSum
{
  static const char *mc_timer_names[];

  void start(vector<T> &values, const Player &P);
  void finish(vector<T> &values, const Player &P);

protected:
  int base_player;
  int opening_sum;
  int max_broadcast;
  octetStream os;

  void ReceiveValues(vector<T> &values, const Player &P, int sender);
  virtual void AddToValues(vector<T> &values) { (void)values; }

public:
  vector<octetStream> oss;
  vector<Timer> timers;
  vector<Timer> player_timers;

  TreeSum(int opening_sum = 10, int max_broadcast = 10, int base_player = 0);
  virtual ~TreeSum();

  void run(vector<T> &values, const Player &P);
  T run(const T &value, const Player &P);

  octetStream &get_buffer() { return os; }

  size_t report_size(ReportType type);
};

template <class T>
class TreeVss_Sum
{
  static const char *mc_timer_names[];

  void start(vector<T> &values, const Player &P);
  void finish(vector<T> &values, const Player &P);

protected:
  int base_player;
  int opening_sum;
  int max_broadcast;
  octetStream os;

  void ReceiveValues(vector<T> &values, const Player &P, int sender);
  virtual void AddToValues(vector<T> &values) { (void)values; }

public:
  vector<octetStream> oss;
  vector<Timer> timers;
  vector<Timer> player_timers;

  TreeVss_Sum(int opening_sum = 10, int max_broadcast = 10, int base_player = 0);
  virtual ~TreeVss_Sum();

  void run(vector<T> &values, const Player &P);
  T run(const T &value, const Player &P);

  octetStream &get_buffer() { return os; }

  size_t report_size(ReportType type);
};

/* 新增vss-field*/
template <class T>
class TreeVssField_Sum
{
  static const char *mc_timer_names[];

  void start(vector<T> &values, const Player &P);
  void finish(vector<T> &values, const Player &P);

protected:
  int base_player;
  int opening_sum;
  int max_broadcast;
  octetStream os;

  void ReceiveValues(vector<T> &values, const Player &P, int sender);
  virtual void AddToValues(vector<T> &values) { (void)values; }

public:
  vector<octetStream> oss;
  vector<Timer> timers;
  vector<Timer> player_timers;
  vector<vector<int>> public_matrix;
  vector<T> field_inv; // 恢复系数

  TreeVssField_Sum(int opening_sum = 10, int max_broadcast = 10, int base_player = 0);
  virtual ~TreeVssField_Sum();

  void run(vector<T> &values, const Player &P);
  T run(const T &value, const Player &P);

  octetStream &get_buffer() { return os; }

  size_t report_size(ReportType type);

  Integer determinant(vector<vector<int>> &matrix);             // 行列式
  vector<vector<T>> adjointMatrix(vector<vector<int>> &matrix); // 伴随矩阵
};

template <class U>
class Tree_MAC_Check : public TreeSum<typename U::open_type>, public MAC_Check_Base<U>
{
  typedef typename U::open_type T;

  template <class V>
  friend class Tree_MAC_Check;

protected:
  static Coordinator *coordinator;

  /* POpen Data */
  int popen_cnt;
  vector<typename U::mac_type> macs;
  vector<T> vals;

  void AddToValues(vector<T> &values);
  void CheckIfNeeded(const Player &P);
  int WaitingForCheck()
  {
    return max(macs.size(), vals.size());
  }

public:
  static void setup(Player &P);
  static void teardown();

  Tree_MAC_Check(const typename U::mac_key_type::Scalar &ai, int opening_sum = 10,
                 int max_broadcast = 10, int send_player = 0);
  virtual ~Tree_MAC_Check();

  virtual void init_open(const Player &P, int n = 0);
  virtual void prepare_open(const U &secret, int = -1);
  virtual void exchange(const Player &P);

  virtual void AddToCheck(const U &share, const T &value, const Player &P);
  virtual void Check(const Player &P) = 0;

  // compatibility
  void set_random_element(const U &random_element) { (void)random_element; }
};

template <class U>
Coordinator *Tree_MAC_Check<U>::coordinator = 0;

/**
 * SPDZ opening protocol with MAC check (indirect communication)
 */
template <class U>
class MAC_Check_ : public Tree_MAC_Check<U>
{
public:
  MAC_Check_(const typename U::mac_key_type::Scalar &ai, int opening_sum = 10,
             int max_broadcast = 10, int send_player = 0);
  virtual ~MAC_Check_() {}

  virtual void Check(const Player &P);
};

template <class T>
using MAC_Check = MAC_Check_<Share<T>>;

template <int K, int S>
class Spdz2kShare;
template <class T>
class Spdz2kPrep;
template <class T>
class MascotPrep;

/**
 * SPDZ2k opening protocol with MAC check
 */
template <class T, class U, class V, class W>
class MAC_Check_Z2k : public Tree_MAC_Check<W>
{
protected:
  Preprocessing<W> *prep;

  W get_random_element();

public:
  vector<W> random_elements;

  MAC_Check_Z2k(const T &ai, int opening_sum = 10, int max_broadcast = 10, int send_player = 0);
  MAC_Check_Z2k(const T &ai, Names &Nms, int thread_num);

  void prepare_open(const W &secret, int = -1);
  void prepare_open_no_mask(const W &secret);

  virtual void Check(const Player &P);
  void set_random_element(const W &random_element);
  void set_prep(Preprocessing<W> &prep);
  virtual ~MAC_Check_Z2k(){};
};

template <class W>
using MAC_Check_Z2k_ = MAC_Check_Z2k<typename W::open_type,
                                     typename W::mac_key_type, typename W::open_type, W>;

template <class T>
void add_openings(vector<T> &values, const Player &P, int sum_players, int last_sum_players, int send_player, TreeSum<T> &MC);

/**
 * SPDZ opening protocol with MAC check (pairwise communication)
 */
template <class T>
class Direct_MAC_Check : public MAC_Check_<T>
{
  typedef MAC_Check_<T> super;

  typedef typename T::open_type open_type;

  int open_counter;
  vector<octetStream> oss;

protected:
  void pre_exchange(const Player &P);

public:
  // legacy interface
  Direct_MAC_Check(const typename T::mac_key_type::Scalar &ai, Names &Nms, int thread_num);
  Direct_MAC_Check(const typename T::mac_key_type::Scalar &ai);
  ~Direct_MAC_Check();

  void init_open(const Player &P, int n = 0);
  void prepare_open(const T &secret, int = -1);
  void exchange(const Player &P);
};

enum mc_timer
{
  SEND,
  RECV_ADD,
  BCAST,
  RECV_SUM,
  SEED,
  COMMIT,
  WAIT_SUMMER,
  RECV,
  SUM,
  SELECT,
  MAX_TIMER
};

template <class T>
TreeSum<T>::TreeSum(int opening_sum, int max_broadcast, int base_player) : base_player(base_player), opening_sum(opening_sum), max_broadcast(max_broadcast)
{
  timers.resize(MAX_TIMER);
}

template <class T>
TreeSum<T>::~TreeSum()
{
#ifdef TREESUM_TIMINGS
  for (unsigned int i = 0; i < timers.size(); i++)
    if (timers[i].elapsed() > 0)
      cerr << T::type_string() << " " << mc_timer_names[i] << ": "
           << timers[i].elapsed() << endl;

  for (unsigned int i = 0; i < player_timers.size(); i++)
    if (player_timers[i].elapsed() > 0)
      cerr << T::type_string() << " waiting for " << i << ": "
           << player_timers[i].elapsed() << endl;
#endif
}

template <class T>
void TreeSum<T>::run(vector<T> &values, const Player &P)
{
  start(values, P);
  finish(values, P);
}

template <class T>
T TreeSum<T>::run(const T &value, const Player &P)
{
  vector<T> values = {value};
  run(values, P);
  return values[0];
}

template <class T>
size_t TreeSum<T>::report_size(ReportType type)
{
  if (type == CAPACITY)
    return os.get_max_length();
  else
    return os.get_length();
}

template <class T>
void add_openings(vector<T> &values, const Player &P, int sum_players, int last_sum_players, int send_player, TreeSum<T> &MC)
{
  MC.player_timers.resize(P.num_players());
  vector<octetStream> &oss = MC.oss;
  oss.resize(P.num_players());
  vector<int> senders;
  senders.reserve(P.num_players());

  for (int relative_sender = positive_modulo(P.my_num() - send_player, P.num_players()) + sum_players;
       relative_sender < last_sum_players; relative_sender += sum_players)
  {
    int sender = positive_modulo(send_player + relative_sender, P.num_players());
    senders.push_back(sender);
  }

  for (int j = 0; j < (int)senders.size(); j++)
    P.request_receive(senders[j], oss[j]);

  for (int j = 0; j < (int)senders.size(); j++)
  {
    int sender = senders[j];
    MC.player_timers[sender].start();
    P.wait_receive(sender, oss[j]);
    MC.player_timers[sender].stop();
    MC.timers[SUM].start();
    for (unsigned int i = 0; i < values.size(); i++)
    {
      values[i].add(oss[j]);
    }
    MC.timers[SUM].stop();
  }
}

template <class T>
void TreeSum<T>::start(vector<T> &values, const Player &P)
{
  os.reset_write_head();
  int sum_players = P.num_players();
  int my_relative_num = positive_modulo(P.my_num() - base_player, P.num_players());
  while (true)
  {
    // summing phase
    int last_sum_players = sum_players;
    sum_players = (sum_players - 2 + opening_sum) / opening_sum;
    if (sum_players == 0)
      break;
    if (my_relative_num >= sum_players && my_relative_num < last_sum_players)
    {
      // send to the player up the tree
      for (unsigned int i = 0; i < values.size(); i++)
      {
        values[i].pack(os);
      }
      int receiver = positive_modulo(base_player + my_relative_num % sum_players, P.num_players());
      timers[SEND].start();
      P.send_to(receiver, os);
      timers[SEND].stop();
    }

    if (my_relative_num < sum_players)
    {
      // if receiving, add the values
      timers[RECV_ADD].start();
      add_openings<T>(values, P, sum_players, last_sum_players, base_player, *this);
      timers[RECV_ADD].stop();
    }
  }

  if (P.my_num() == base_player)
  {
    // send from the root player
    os.reset_write_head();
    size_t n = values.size();
    for (unsigned int i = 0; i < n; i++)
    {
      values[i].pack(os);
    }
    timers[BCAST].start();
    for (int i = 1; i < max_broadcast && i < P.num_players(); i++)
    {
      P.send_to((base_player + i) % P.num_players(), os);
    }
    timers[BCAST].stop();
    AddToValues(values);
  }
  else if (my_relative_num * max_broadcast < P.num_players())
  {
    // send if there are children
    int sender = (base_player + my_relative_num / max_broadcast) % P.num_players();
    ReceiveValues(values, P, sender);
    timers[BCAST].start();
    for (int i = 0; i < max_broadcast; i++)
    {
      int relative_receiver = (my_relative_num * max_broadcast + i);
      if (relative_receiver < P.num_players())
      {
        int receiver = (base_player + relative_receiver) % P.num_players();
        P.send_to(receiver, os);
      }
    }
    timers[BCAST].stop();
  }
}

template <class T>
void TreeSum<T>::finish(vector<T> &values, const Player &P)
{
  int my_relative_num = positive_modulo(P.my_num() - base_player, P.num_players());
  if (my_relative_num * max_broadcast >= P.num_players())
  {
    // receiving at the leafs
    int sender = (base_player + my_relative_num / max_broadcast) % P.num_players();
    ReceiveValues(values, P, sender);
  }
}

template <class T>
void TreeSum<T>::ReceiveValues(vector<T> &values, const Player &P, int sender)
{
  timers[RECV_SUM].start();
  P.receive_player(sender, os);
  timers[RECV_SUM].stop();
  for (unsigned int i = 0; i < values.size(); i++)
    values[i].unpack(os);
  AddToValues(values);
}

template <class T>
TreeVss_Sum<T>::TreeVss_Sum(int opening_sum, int max_broadcast, int base_player) : base_player(base_player), opening_sum(opening_sum), max_broadcast(max_broadcast)
{
  timers.resize(MAX_TIMER);
}

template <class T>
TreeVss_Sum<T>::~TreeVss_Sum()
{
#ifdef TREESUM_TIMINGS
  for (unsigned int i = 0; i < timers.size(); i++)
    if (timers[i].elapsed() > 0)
      cerr << T::type_string() << " " << mc_timer_names[i] << ": "
           << timers[i].elapsed() << endl;

  for (unsigned int i = 0; i < player_timers.size(); i++)
    if (player_timers[i].elapsed() > 0)
      cerr << T::type_string() << " waiting for " << i << ": "
           << player_timers[i].elapsed() << endl;
#endif
}

template <class T>
void TreeVss_Sum<T>::run(vector<T> &values, const Player &P)
{
  start(values, P);
  finish(values, P);
}

template <class T>
T TreeVss_Sum<T>::run(const T &value, const Player &P)
{
  vector<T> values = {value};
  run(values, P);
  return values[0];
}

template <class T>
size_t TreeVss_Sum<T>::report_size(ReportType type)
{
  if (type == CAPACITY)
    return os.get_max_length();
  else
    return os.get_length();
}

template <class T>
void add_Vss_openings(vector<T> &values, const Player &P, int sum_players, int last_sum_players, int send_player, TreeVss_Sum<T> &MC)
{
  MC.player_timers.resize(P.num_players());
  vector<octetStream> &oss = MC.oss;
  oss.resize(P.num_players());
  vector<int> senders;
  senders.reserve(P.num_players());

  for (int relative_sender = positive_modulo(P.my_num() - send_player, P.num_players()) + sum_players; // 正模
       relative_sender < last_sum_players; relative_sender += sum_players)
  {
    int sender = positive_modulo(send_player + relative_sender, P.num_players());
    senders.push_back(sender);
  }

  for (int j = 0; j < (int)senders.size(); j++)
    P.request_receive(senders[j], oss[j]);

  for (int j = 0; j < (int)senders.size(); j++)
  {
    int sender = senders[j];
    MC.player_timers[sender].start();
    P.wait_receive(sender, oss[j]);
    MC.player_timers[sender].stop();
    MC.timers[SUM].start();
    for (unsigned int i = 0; i < values.size(); i++)
    {
      values[i].vss_add(oss[j], P, j);
    }
    MC.timers[SUM].stop();
  }
}

template <class T>
void TreeVss_Sum<T>::start(vector<T> &values, const Player &P)
{
  os.reset_write_head();
  int sum_players = P.num_players();
  int my_relative_num = positive_modulo(P.my_num() - base_player, P.num_players());
  while (true)
  {
    // summing phase
    int last_sum_players = sum_players;
    sum_players = (sum_players - 2 + opening_sum) / opening_sum;
    if (sum_players == 0)
      break;
    if (my_relative_num >= sum_players && my_relative_num < last_sum_players)
    {
      // send to the player up the tree
      for (unsigned int i = 0; i < values.size(); i++)
      {
        values[i].pack(os);
      }
      int receiver = positive_modulo(base_player + my_relative_num % sum_players, P.num_players());
      timers[SEND].start();
      P.send_to(receiver, os);
      timers[SEND].stop();
    }

    if (my_relative_num < sum_players)
    {
      // if receiving, add the values
      timers[RECV_ADD].start();
      add_Vss_openings<T>(values, P, sum_players, last_sum_players, base_player, *this); // 累加获得values
      timers[RECV_ADD].stop();
    }
  }

  if (P.my_num() == base_player)
  {
    // send from the root player
    os.reset_write_head();
    size_t n = values.size();
    for (unsigned int i = 0; i < n; i++)
    {
      values[i].pack(os);
    }
    timers[BCAST].start();
    for (int i = 1; i < max_broadcast && i < P.num_players(); i++)
    {
      P.send_to((base_player + i) % P.num_players(), os);
    }
    timers[BCAST].stop();
    AddToValues(values);
  }
  else if (my_relative_num * max_broadcast < P.num_players())
  {
    // send if there are children
    int sender = (base_player + my_relative_num / max_broadcast) % P.num_players();
    ReceiveValues(values, P, sender);
    timers[BCAST].start();
    for (int i = 0; i < max_broadcast; i++)
    {
      int relative_receiver = (my_relative_num * max_broadcast + i);
      if (relative_receiver < P.num_players())
      {
        int receiver = (base_player + relative_receiver) % P.num_players();
        P.send_to(receiver, os);
      }
    }
    timers[BCAST].stop();
  }
}

template <class T>
void TreeVss_Sum<T>::finish(vector<T> &values, const Player &P)
{
  int my_relative_num = positive_modulo(P.my_num() - base_player, P.num_players());
  if (my_relative_num * max_broadcast >= P.num_players())
  {
    // receiving at the leafs
    int sender = (base_player + my_relative_num / max_broadcast) % P.num_players();
    ReceiveValues(values, P, sender);
  }
}

template <class T>
void TreeVss_Sum<T>::ReceiveValues(vector<T> &values, const Player &P, int sender)
{
  timers[RECV_SUM].start();
  P.receive_player(sender, os);
  timers[RECV_SUM].stop();
  for (unsigned int i = 0; i < values.size(); i++)
    values[i].unpack(os);
  AddToValues(values);
}

/* 新增vss-field */

template <class T>
TreeVssField_Sum<T>::TreeVssField_Sum(int opening_sum, int max_broadcast, int base_player) : base_player(base_player), opening_sum(opening_sum), max_broadcast(max_broadcast)
{
  timers.resize(MAX_TIMER);
}

template <class T>
TreeVssField_Sum<T>::~TreeVssField_Sum()
{
#ifdef TREESUM_TIMINGS
  for (unsigned int i = 0; i < timers.size(); i++)
    if (timers[i].elapsed() > 0)
      cerr << T::type_string() << " " << mc_timer_names[i] << ": "
           << timers[i].elapsed() << endl;

  for (unsigned int i = 0; i < player_timers.size(); i++)
    if (player_timers[i].elapsed() > 0)
      cerr << T::type_string() << " waiting for " << i << ": "
           << player_timers[i].elapsed() << endl;
#endif
}

template <class T>
void TreeVssField_Sum<T>::run(vector<T> &values, const Player &P)
{
  // cout<<"我在TreeVssField_Sum的run函数"<<endl;
  start(values, P);
  finish(values, P);
}

template <class T>
T TreeVssField_Sum<T>::run(const T &value, const Player &P)
{
  vector<T> values = {value};
  run(values, P);
  return values[0];
}

template <class T>
size_t TreeVssField_Sum<T>::report_size(ReportType type)
{
  if (type == CAPACITY)
    return os.get_max_length();
  else
    return os.get_length();
}

template <class T>
void add_Vss_Field_openings(vector<T> &values, const Player &P, int sum_players, int last_sum_players, int send_player, TreeVssField_Sum<T> &MC)
{
  // for (unsigned int i = 0; i < values.size(); i++)
  // {
  //   values[i] *= MC.field_inv[0]; // 乘第一个恢复系数
  // }
  MC.player_timers.resize(P.num_players());
  vector<octetStream> &oss = MC.oss;
  oss.resize(P.num_players());
  vector<int> senders;
  senders.reserve(P.num_players()); // 分配n的内存空间

  for (int relative_sender = positive_modulo(P.my_num() - send_player, P.num_players()) + sum_players; // 正模
       relative_sender < last_sum_players; relative_sender += sum_players)                             // 循环条件是relative_sender < last_sum_players
  {
    int sender = positive_modulo(send_player + relative_sender, P.num_players());
    senders.push_back(sender);
  } // 计算一组发送者的编号，预计需更改

  for (int j = 0; j < (int)senders.size(); j++)
    P.request_receive(senders[j], oss[j]);

  for (int j = 0; j < (int)senders.size(); j++)
  {
    int sender = senders[j];
    MC.player_timers[sender].start();
    P.wait_receive(sender, oss[j]);
    MC.player_timers[sender].stop();
    MC.timers[SUM].start();
    for (unsigned int i = 0; i < values.size(); i++)
    {
      // cout << endl <<"初始时values["<<i<<"]:"<<values[i]<<endl;
      values[i].vss_add(oss[j], P, MC.field_inv, j); // j 是 sender ，value[i]+ field_inv[sender] * os里的value
      // cout<<"values["<<i<<"]:"<<values[i]<<endl;
    }
    MC.timers[SUM].stop();
  }
}

// 求矩阵的行列式
template <class T>
Integer TreeVssField_Sum<T>::determinant(vector<vector<int>> &matrix)
{
  int n = matrix.size();
  if (n == 2)
  {
    Integer det = (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]);
    return det;
  }
  Integer det = 0;
  bool sign = true;
  for (int i = 0; i < n; i++)
  {
    vector<vector<int>> submatrix(n - 1, vector<int>(n - 1));
    for (int j = 1; j < n; j++)
    {
      int col = 0;
      for (int k = 0; k < n; k++)
      {
        if (k != i)
        {
          submatrix[j - 1][col] = matrix[j][k];
          col++;
        }
      }
    }
    if (sign == true)
      det = det + (determinant(submatrix) * matrix[0][i]);
    else
      det = det - (determinant(submatrix) * matrix[0][i]);
    sign = !sign;
  }
  return det;
}

// 求矩阵的伴随矩阵
template <class T>
vector<vector<T>> TreeVssField_Sum<T>::adjointMatrix(vector<vector<int>> &matrix)
{
  int n = matrix.size();
  vector<vector<T>> adj(n, vector<T>(n));
  for (int i = 0; i < n; i++)
  {
    for (int j = 0; j < n; j++)
    {
      vector<vector<int>> submatrix(n - 1, vector<int>(n - 1));
      int subi = 0, subj = 0;
      for (int k = 0; k < n; k++)
      {
        if (k != i)
        {
          subj = 0;
          for (int l = 0; l < n; l++)
          {
            if (l != j)
            {
              submatrix[subi][subj] = matrix[k][l];
              subj++;
            }
          }
          subi++;
        }
      }
      int sign = ((i + j) % 2 == 0) ? 1 : -1;
      adj[j][i] = Integer(sign) * determinant(submatrix);
    }
  }
  return adj;
}

template <class T>
void TreeVssField_Sum<T>::start(vector<T> &values, const Player &P)
{
  int public_matrix_row = P.num_players(); // n+nd
  // int public_matrix_col = P.num_players() - ndparties; // n
  int public_matrix_col = P.num_players(); // n+nd

  public_matrix.resize(public_matrix_row);
  field_inv.resize(public_matrix_col);

  for (int i = 0; i < public_matrix_row; i++)
  {
    public_matrix[i].resize(public_matrix_col);
  }
  for (int i = 0; i < public_matrix_row; i++)
  {
    int x = 1;
    public_matrix[i][0] = 1;
    for (int j = 1; j < public_matrix_col; j++)
    {
      x *= (i + 1);
      public_matrix[i][j] = x;
    }
  }

  // 求前n行的行列式
  vector<vector<int>> selected(public_matrix.begin(), public_matrix.begin() + public_matrix_col);
  T det = determinant(selected);                   // 行列式
  T det_inv = det.invert();                        // 行列式的逆
  vector<vector<T>> adj = adjointMatrix(selected); // 伴随矩阵
  for (int i = 0; i < public_matrix_col; i++)
  {
    field_inv[i] = adj[0][i] * det_inv; // 逆矩阵的第一行
  }
  
  T inv0 = field_inv[0];
  for (int i = 0; i < public_matrix_col; i++)
  {
    field_inv[i] = field_inv[i] / inv0; // 第一个恢复系数
  }

  os.reset_write_head();
  int sum_players = P.num_players();
  int my_relative_num = positive_modulo(P.my_num() - base_player, P.num_players());
  while (true)
  {
    // summing phase
    int last_sum_players = sum_players;
    sum_players = (sum_players - 2 + opening_sum) / opening_sum;
    if (sum_players == 0)
      break;
    if (my_relative_num >= sum_players && my_relative_num < last_sum_players)
    {
      // send to the player up the tree
      for (unsigned int i = 0; i < values.size(); i++)
      {
        values[i].pack(os);
      }
      int receiver = positive_modulo(base_player + my_relative_num % sum_players, P.num_players());
      timers[SEND].start();
      P.send_to(receiver, os);
      timers[SEND].stop();
    }

    if (my_relative_num < sum_players)
    {
      // if receiving, add the values
      timers[RECV_ADD].start();
      add_Vss_Field_openings<T>(values, P, sum_players, last_sum_players, base_player, *this); // 累加获得values
      timers[RECV_ADD].stop();
    }
  }

  if (P.my_num() == base_player)
  {
    // send from the root player
    os.reset_write_head();
    size_t n = values.size();
    for (unsigned int i = 0; i < n; i++)
    {
      values[i].pack(os);
    }
    timers[BCAST].start();
    for (int i = 1; i < max_broadcast && i < P.num_players(); i++)
    {
      P.send_to((base_player + i) % P.num_players(), os);
    }
    timers[BCAST].stop();
    AddToValues(values);
  }
  else if (my_relative_num * max_broadcast < P.num_players())
  {
    // send if there are children
    int sender = (base_player + my_relative_num / max_broadcast) % P.num_players();
    ReceiveValues(values, P, sender);
    timers[BCAST].start();
    for (int i = 0; i < max_broadcast; i++)
    {
      int relative_receiver = (my_relative_num * max_broadcast + i);
      if (relative_receiver < P.num_players())
      {
        int receiver = (base_player + relative_receiver) % P.num_players();
        P.send_to(receiver, os);
      }
    }
    timers[BCAST].stop();
  }
  // for (unsigned int i = 0; i < values.size(); i++)
  // {
  //   values[i] *= inv0;
  // }
}

template <class T>
void TreeVssField_Sum<T>::finish(vector<T> &values, const Player &P)
{
  int my_relative_num = positive_modulo(P.my_num() - base_player, P.num_players());
  if (my_relative_num * max_broadcast >= P.num_players())
  {
    // receiving at the leafs
    int sender = (base_player + my_relative_num / max_broadcast) % P.num_players();
    ReceiveValues(values, P, sender);
  }
}

template <class T>
void TreeVssField_Sum<T>::ReceiveValues(vector<T> &values, const Player &P, int sender)
{
  timers[RECV_SUM].start();
  P.receive_player(sender, os);
  timers[RECV_SUM].stop();
  for (unsigned int i = 0; i < values.size(); i++)
    values[i].unpack(os);
  AddToValues(values);
}
#endif