#include "Table.h"

using namespace tiny_dnn;

Table Table::score(tiny_dnn::network<tiny_dnn::sequential> net, Table field)
{
	int field_w = field.width;
	int field_h = field.height;

	vec_t pos_item = field.toVec();

	vec_t prior = net.predict(pos_item);
	Table res = Table::fromVec(prior, field_w, field_h, 1);

	return res;
}