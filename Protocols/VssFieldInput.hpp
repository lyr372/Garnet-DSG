/*
 * VssFieldInput.cpp
 *
 */

#ifndef PROTOCOLS_VSSFIELDINPUT_HPP_
#define PROTOCOLS_VSSFIELDINPUT_HPP_

#include "VssFieldInput.h"

template <class T>
VssFieldInput<T>::VssFieldInput(SubProcessor<T> *proc, Player &P) : SemiInput<T>(proc, P), P(P)
{
    // cout << "进入Input构造函数" << endl;
    ndparties = VssFieldMachine::s().ndparties;
    int public_matrix_row = P.num_players(); // n+nd
    // int public_matrix_col = P.num_players() - ndparties; // n
    int public_matrix_col = P.num_players(); // n+nd, 为了测试，暂时设为n+nd

    os.resize(2); // 是什么，socket发送
    os[0].resize(public_matrix_col);
    os[1].resize(public_matrix_col);
    expect.resize(public_matrix_col); // 是什么

    public_matrix.resize(public_matrix_row);
    P.public_matrix.resize(public_matrix_row);
    for (int i = 0; i < public_matrix_row; i++)
    {
        public_matrix[i].resize(public_matrix_col);
        P.public_matrix[i].resize(public_matrix_col);
    }
    for (int i = 0; i < public_matrix_row; i++)
    {
        int x = 1;
        public_matrix[i][0] = 1;
        P.public_matrix[i][0] = 1;
        for (int j = 1; j < public_matrix_col; j++)
        {
            x *= (i + 1);
            public_matrix[i][j] = x;
            P.public_matrix[i][j] = x;
        }
    }

    // test inv
    // int array[4][3] = {{1, 0, 1},
    //                    {2, 2, -3},
    //                    {3, 3, -4},
    //                    {1, 1, -1}};

    // for (int i = 0; i < 3; i++)
    // {
    //     for (int j = 0; j < 3; j++)
    //     {
    //         P.public_matrix[i][j] = array[i][j];
    //         public_matrix[i][j] = array[i][j];
    //     }
    // }
    // cout << "结束" << endl;
    this->reset_all(P);
}

template <class T>
void VssFieldInput<T>::reset(int player)
{
    if (player == P.my_num())
    {
        this->shares.clear();
        os.resize(2);
        for (int i = 0; i < 2; i++)
        {
            os[i].resize(public_matrix[0].size());
            for (auto &o : os[i])
                o.reset_write_head();
        }
    }
    expect[player] = false;
}

template <class T>
void VssFieldInput<T>::add_mine(const typename T::clear &input, int) // 计算秘密份额
{
    auto &P = this->P;
    vector<typename T::open_type> v(public_matrix[0].size());
    vector<T> secrets(public_matrix.size());
    PRNG G;
    v[0] = input;
    for (int i = 1; i < public_matrix[0].size(); i++)
    {
        // v[i] = G.get<typename T::open_type>(); // for test,记得改回来
        v[i] = i;
    }
    for (int i = 0; i < public_matrix.size(); i++)
    {
        typename T::open_type sum = 0;
        for (int j = 0; j < public_matrix[0].size(); j++)
        {
            sum += v[j] * public_matrix[i][j];
        }
        secrets[i] = sum;
        // cout<<"secrets["<<i<<"]:"<<secrets[i]<<endl;
    }
    this->shares.push_back(secrets[P.my_num()]);
    for (int i = 0; i < P.num_players(); i++)
    {
        if (i != P.my_num())
        {
            secrets[i].pack(os[0][i]);
        }
    }

    // typename T::open_type sum;
    // std::vector<typename T::open_type> shares(P.num_players());
    // for (int i = 0; i < P.num_players(); i++)
    // {
    //     if (i != P.my_num())
    //     {
    //         sum += this->send_prngs[i].template get<typename T::open_type>() * P.inv[i];
    //         // sum += this->send_prngs[i].template get<typename T::open_type>();
    //     }
    // }
    // cout << sum <<endl;
    // bigint temp = input - sum;
    // stringstream ss;
    // ss << temp;
    // long value = stol(ss.str()) / P.inv[P.my_num()];
    // cout << value << endl;
    // this->shares.push_back(value);
}

template <class T>
void VssFieldInput<T>::add_other(int player, int)
{
    expect[player] = true;
}

template <class T>
void VssFieldInput<T>::exchange()
{
    if (!os[0][(P.my_num() + 1) % P.num_players()].empty()) // 如果不为空
    {
        for (int i = 0; i < P.num_players(); i++)
        {
            if (i != P.my_num())
                {
                    P.send_to(i, os[0][i]); // 发送数据(秘密份额)
                }
        }
        
        for (int i = 0; i < P.num_players(); i++)
        {
            if (expect[i]) // 从expect[i]的参与者处接收数据
            {
                P.receive_player(i, os[1][i]);
            }
        }
    }
    else // 如果为空，无需发送数据
    {
        for (int i = 0; i < P.num_players(); i++)
        {
            if (expect[i])
                P.receive_player(i, os[1][i]);
        }
    }
}

template <class T>
void VssFieldInput<T>::finalize_other(int player, T &target, octetStream &,
                                      int)
// 从其他参与者那里接收的数据存到target中
{
    target = os[1][player].template get<T>();
}

template <class T>
T VssFieldInput<T>::finalize_mine() // 获取并返回shares的下一个元素？
{
    return this->shares.next();
}

#endif // PROTOCOLS_VSSFIELDINPUT_HPP_
