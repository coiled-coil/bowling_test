#include <vector>
#include <iostream>
#include "coroutine.hpp"
#include "yield.hpp"

using namespace std;

enum {
    SPARE = -1,
    STRIKE = -2,
    FIN = -3,
};

template<
    typename Functor
>
class calculator : coroutine
{
public:
    explicit calculator(int& result, Functor async_get_score0)
    :   result_(result)
    ,   val_()
    ,   game_()
    ,   index_()
    ,   async_get_score(async_get_score0)
    {
    }

    void operator()(int score = 0)
    {
        reenter(this) {
            for (game_ = 0; game_ < 9; ++game_) {
                // １投目
                yield async_get_score(index_++, *this);
                val_ = score;
                if (score == STRIKE) {
                    fork calculator(*this)();
                    if (is_parent()) {
                        continue;
                    }

                    // ２投目
                    yield async_get_score(index_++, *this);
                    val_ = score;

                    // ３投目
                    yield async_get_score(index_++, *this);
                    if (score == SPARE) {
                        result_ += 20;
                    }
                    else {
                        result_ += get_value(STRIKE) + get_value(val_) + get_value(score);
                    }
                    cout << "[" << (game_ + 1) << "] " << result_ << endl;
                    return;
                }

                // ２投目
                yield async_get_score(index_++, *this);
                if (score == SPARE) {
                    fork calculator(*this)();
                    if (is_child()) {
                        // ３投目
                        yield async_get_score(index_++, *this);
                        result_ += get_value(SPARE) + get_value(score);
                        cout << "[" << (game_ + 1) << "] " << result_ << endl;
                        return;
                    }
                }
                else {
                    result_ += get_value(val_) + get_value(score);
                    cout << "[" << (game_ + 1) << "] " << result_ << endl;
                }
            }

            // 10フレーム目は単純に足し算
            yield async_get_score(index_++, *this);
            val_ = get_value(score);

            yield async_get_score(index_++, *this);
            val_ = score == SPARE ? 10 : val_ + get_value(score);

            yield async_get_score(index_++, *this);
            val_ += get_value(score);

            result_ += val_;
            cout << "[" << (game_ + 1) << "] " << result_ << endl;
        }
    }

private:
    int get_value(int score)
    {
        switch (score)
        {
        case SPARE:
        case STRIKE:
            return 10;

        case FIN:
            return 0;

        default:
            return score;
        }
    }

private:
    int& result_;
    int val_, game_, index_;
    Functor async_get_score;
};


struct get_score
{
    vector<int>& scores_;

    explicit get_score(vector<int>& scores)
    :   scores_(scores)
    {}

    template<typename Handler>
    void operator()(int index, Handler handler)
    {
        if (index >= scores_.size()) {
            handler(FIN);
        }
        else {
            // cout << index << ": " << scores_[index] << endl;
            handler(scores_[index]);
        }
    }
};

int main()
{
    vector<int> score_table = {
        0, 0,
        STRIKE,
        8, SPARE,
        STRIKE,
        STRIKE,
        STRIKE,
        5, 3,
        8, SPARE,
        STRIKE,
        STRIKE,
        STRIKE,
        STRIKE,
    };

    int result = 0;
    calculator<get_score> cal(result, get_score(score_table));
    cal();

    cout << result << endl;

    return 1;
    //return result == 201 ? 0 : 1;
}
