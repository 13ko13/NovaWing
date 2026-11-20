#include "Easing.h"

namespace Easing
{
	float EaseInOutQuad(float t)
	{
		//前半は加速させ、後半は減速させる
		//t * tした場合は半分の時間で0.25しか進まないので2倍して0.5に合わせる
		//後半は残り時間(1-t)を使っているので残りの道のりを出して、1から引いて進んだ道のりを返す
		//前半は出発から測った道のりなので1から引かずにそのまま返す
		if (t < 0.5f) return 2 * t * t;
		else return 1 - 2 * (1 - t) * (1 - t);
	}
}
