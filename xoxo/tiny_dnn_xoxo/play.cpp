#include <stdio.h>

#include "tiny_dnn/tiny_dnn.h"

#include <memory>
#include <vector>
#include <fstream>

using namespace tiny_dnn;

using namespace std;

void printField(vector<float> field_data, int field_w, int field_h) {
	printf("   ");
	for (int i = 0; i < field_w; i++)
	{
		printf("%c ", 'a' + i);
	}
	printf("\n");

	for (int j = 0; j < field_h; j++)
	{
		printf("%2d", j + 1);
		for (int i = 0; i < field_w; i++)
		{

			if (field_data[(j * field_w + i) * 2 + 0] > 0.5) {
				printf(" X");
			}
			else if (field_data[(j * field_w + i) * 2 + 1] > 0.5)
			{
				printf(" O");
			}
			else
			{
				printf(" .");
			}
			//field_data[j * field_w + i]
		}
		printf("\n");
	}
	printf("\n");

}

void printPriors(vector<float> field_data, int field_w, int field_h)
{
	for (int j = 0; j < field_h; j++)
	{
		for (int i = 0; i < field_w; i++)
		{
			if (field_data[j * field_w + i] < -99) {
				printf("XXXX ");
			} else {
				printf("%.2f ", field_data[j * field_w + i]);
			}
		}
		printf("\n");
	}
	printf("\n");
}


bool playerCond(int pI, float* val) {
	return val[pI] > 0.5;
}

//float playerVal(int pI) {
//	if (pI == 0 /*X*/) return 1.0;
//	else if (pI == 1 /*O*/) return -1.0;
//	else throw 0;
//}

char playerSymbol(int pI) {
	if (pI == 0 /*X*/) return 'X';
	else if (pI == 1 /*O*/) return 'O';
	else throw 0;
}

int checkVictory(vector<float> field_data, int field_w, int field_h, int line_len) {
	for (int j = 0; j < field_h + line_len - 1; j++) {
		for (int i = 0; i < field_w + line_len - 1; i++) {
			int i1 = i % field_w;
			int j1 = j % field_h;

			// Iterate thru players - 0=X 1=O
			for (int pI = 0; pI < 2; pI++) {
				int cnt;
				// Checking player victory
				if (playerCond(pI, &field_data[(j1 * field_w + i1) * 2])) {

					// Right
					cnt = 1;
					for (int p = 1; p < line_len; p++) {
						int i2 = (i + p) % field_w;
						if (playerCond(pI, &field_data[(j1 * field_w + i2) * 2])) cnt++;
					}
					if (cnt == line_len)
						return pI;

					// Down
					cnt = 1;
					for (int q = 1; q < line_len; q++) {
						int j2 = (j + q) % field_h;
						if (playerCond(pI, &field_data[(j2 * field_w + i1) * 2])) cnt++;
					}
					if (cnt == line_len) 
						return pI;

					// Right-down
					cnt = 1;
					for (int pq = 1; pq < line_len; pq++) {
						int i2 = (i + pq) % field_w;
						int j2 = (j + pq) % field_h;
						if (playerCond(pI, &field_data[(j2 * field_w + i2) * 2])) cnt++;
					}
					if (cnt == line_len)
						return pI;

					// Left-down
					cnt = 1;
					for (int pq = 1; pq < line_len; pq++) {
						int i2 = (i - pq + field_w) % field_w;
						int j2 = (j + pq) % field_h;
						if (playerCond(pI, &field_data[(j2 * field_w + i2) * 2])) cnt++;
					}
					if (cnt == line_len)
						return pI;
				}
			}

		}
	}
	return -1; // No victory found
}

class Point {
public:
	int i, j, width, height;

	Point(int i, int j, int width, int height) : i(i), j(j), width(width), height(height) { }

	Point rotateClockwise() const
	{
		Point res(*this);
		assert(width == height);

		res.i = width - 1 - j;
		res.j = i;

		return res;
	}

	Point mirrorHorizontal() const
	{
		Point res(*this);

		res.i = width - 1 - i;
		res.j = j;

		return res;
	}

	/*Point translateRoll(int dx, int dy)
	{
		Point res(*this);
		res.i = (i + dx + width) % width;
		res.j = (j + dy + height) % height;
		return res;
	}*/
};

class Table
{
public:
	int width, height;
	vector<float> vals;

	static Table empty(int width, int height) {
		vector<float> vals(width * height, 0.0);
		return Table(vals, width, height);
	}

	Table(vector<float> vals, int width, int height) : vals(vals), width(width), height(height) { }
	Table(int width, int height) : width(width), height(height) { }

	Table inverseChannels() const
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				float tmp = res.vals[(j * width + i) * 2 + 0];
				res.vals[(j * width + i) * 2 + 0] = res.vals[(j * width + i) * 2 + 1];
				res.vals[(j * width + i) * 2 + 1] = tmp;
			}
		}

		return res;
	}

	Table rotateClockwise() const
	{
		Table res(*this);
		assert(width == height);
		
		int fd = width;

		for (int j = 0; j < fd; j++) {
			for (int i = 0; i < fd; i++) {
				res.vals[(fd * i + (fd - 1 - j)) * 2 + 0] = vals[(j * fd + i) * 2 + 0];
				res.vals[(fd * i + (fd - 1 - j)) * 2 + 1] = vals[(j * fd + i) * 2 + 1];
			}
		}

		return res;
	}

	Table mirrorHorizontal() const
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				res.vals[(j * width + i) * 2 + 0] = vals[(j * width + (width - 1 - i)) * 2 + 0];
				res.vals[(j * width + i) * 2 + 1] = vals[(j * width + (width - 1 - i)) * 2 + 1];
			}
		}

		return res;
	}

	Table multiply(float priority_mul) const
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				res.vals[(j * width + i) * 2 + 0] *= priority_mul;
				res.vals[(j * width + i) * 2 + 1] *= priority_mul;
			}
		}

		return res;
	}

	Table translateRoll(int dx, int dy)
	{
		Table res(*this);

		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {

				int i2 = (i + dx + width) % width;
				int j2 = (j + dy + height) % height;

				res.vals[(j2 * width + i2) * 2 + 0] = vals[(j * width + i) * 2 + 0];
				res.vals[(j2 * width + i2) * 2 + 1] = vals[(j * width + i) * 2 + 1];
			}
		}

		return res;
	}

	vec_t toVec() {
		vec_t pos_item;
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				for (int c = 0; c < 2; c++) {
					pos_item.push_back(vals[(j * width + i) * 2 + c]);
				}
			}
		}
		return pos_item;
	}
};

/*Table max(Table one, Table two)
{
	assert(one.width == two.width);
	assert(one.height == two.height);

	vector<float> rmax;
	rmax.resize(one.width * one.height);
	for (int j = 0; j < one.height; j++) {
		for (int i = 0; i < one.width; i++) {
			rmax[j * one.width + i] = fmaxf(one.vals[j * one.width + i], two.vals[j * one.width + i]);
		}
	}
	return Table(rmax, one.width, one.height);
}*/

class Lesson
{
private:
	static int dx(int field_w, int i)
	{
		int cx = (field_w - 1) / 2;
		return -(i - cx);
	}
	static int dy(int field_h, int j)
	{
		int cy = (field_h - 1) / 2;
		return -(j - cy);
	}

public:
	int field_w, field_h;
	float priority;
	Table position;

	Lesson(Table position, int field_w, int field_h, Point move, float priority) :
		field_w(field_w), field_h(field_h), priority(priority),
		position(position.translateRoll(dx(field_w, move.i), dy(field_h, move.j)))
	{
	}

	Lesson(Table position, int field_w, int field_h, float priority) :
		field_w(field_w), field_h(field_h), priority(priority),
		position(position)
	{
	}

	Lesson inverseChannels() const
	{
		Lesson res(*this);

		res.position = position.inverseChannels();

		return res;
	}

	Lesson rotateClockwise() const
	{
		Lesson res(*this);
		if (field_w != field_h)
		{
			throw runtime_error("Can't rotate a non-square field");
		}

		res.position = position.rotateClockwise();

		return res;
	}

	Lesson mirrorHorizontal() const
	{
		Lesson res(*this);

		res.position = position.mirrorHorizontal();

		return res;
	}

	Lesson setPriority(float value) const
	{
		Lesson res(*this);

		res.priority *= value;

		return res;
	}

};

float score(network<sequential> net, Table field)
{
	int field_w = field.width;
	int field_h = field.height;

	vec_t pos_item;
	for (int j = 0; j < field_h; j++) {
		for (int i = 0; i < field_w; i++) {
			for (int c = 0; c < 2; c++) {
				pos_item.push_back(field.vals[(j * field_w + i) * 2 + c]);
			}
		}
	}

	vec_t prior = net.predict(pos_item);

	return prior[0];
}

Point makeMove(network<sequential> net, Table field_data, bool rotate_and_mirror = true)
{
	int cx = (field_data.width - 1) / 2;
	int cy = (field_data.height - 1) / 2;

	vector<float> res(field_data.width * field_data.height * 2/* channels */);
	std::fill(res.begin(), res.end(), -100.0f);	// As low as possible
	
	//printf("---> ");

	// Scoring for each cell's priority
	for (int dy = 0; dy < field_data.height; dy++)
	{
		for (int dx = 0; dx < field_data.width; dx++)
		{
			// Rolling the board
			Table rolled = field_data.translateRoll(dx, dy);

			if (abs(rolled.vals[(cy * rolled.width + cx) * 2 + 0]) > 0.5 || 
				abs(rolled.vals[(cy * rolled.width + cx) * 2 + 1]) > 0.5)
			{
				//printf("%d,%d; ", dx, dy);
				continue;	// The cell is occupied
			}

			float ai_prior;

			if (rotate_and_mirror)
			{
				// Rotating & scoring
				float ai_prior_0_0 = score(net, rolled);
				float ai_prior_0_0m = score(net, rolled.mirrorHorizontal());

				Table field_90 = rolled.rotateClockwise();
				float ai_prior_1_90 = score(net, field_90);
				float ai_prior_1_90m = score(net, field_90.mirrorHorizontal());

				Table field_180 = field_90.rotateClockwise();
				float ai_prior_2_180 = score(net, field_180);
				float ai_prior_2_180m = score(net, field_180.mirrorHorizontal());

				Table field_270 = field_180.rotateClockwise();
				float ai_prior_3_270 = score(net, field_270);
				float ai_prior_3_270m = score(net, field_270.mirrorHorizontal());

				float ai_tmp = max(ai_prior_0_0, ai_prior_0_0m);
				ai_tmp = max(ai_tmp, ai_prior_1_90);
				ai_tmp = max(ai_tmp, ai_prior_1_90m);
				ai_tmp = max(ai_tmp, ai_prior_2_180);
				ai_tmp = max(ai_tmp, ai_prior_2_180m);
				ai_tmp = max(ai_tmp, ai_prior_3_270);
				ai_tmp = max(ai_tmp, ai_prior_3_270m);
				ai_prior = ai_tmp;
			}
			else
			{
				ai_prior = score(net, rolled);
			}

			res[dy * field_data.width + dx] = ai_prior;
		}
	}

	vector<float> show = res;
	for (int dy = 0; dy < field_data.height; dy++)
	{
		for (int dx = 0; dx < field_data.width; dx++)
		{
			int xx = (-dx + cx + field_data.width) % field_data.width;
			int yy = (-dy + cy + field_data.height) % field_data.height;

			show[yy * field_data.width + xx] = res[dy * field_data.width + dx];
		}
	}
	printPriors(show, field_data.width, field_data.height);

	// Searching for the hightest priority
	int maxdx = -1, maxdy = -1; float m = -1.0;
	for (int dx = 0; dx < field_data.width; dx++)
	{
		for (int dy = 0; dy < field_data.height; dy++)
		{
			if (res[dy * field_data.width + dx] > m)
			{
				m = res[dy * field_data.width + dx];
				maxdx = dx;
				maxdy = dy;
			}
		}
	}

	//printf(" ==> %d,%d; ", maxdx, maxdy);
	//printf("\n");

	Point move(
		(cx - maxdx + field_data.width) % field_data.width, 
		(cy - maxdy + field_data.height) % field_data.height, 
		field_data.width, field_data.height
	);

	//printf(" ==> %d,%d; ", move.i, move.j);
	//printf("\n");

	return move;
}

void train(int field_w, int field_h, network<sequential> net, float mse_stop)
{
	vector<Lesson> usefulLessons;

	// Loading lessons
	ifstream lessonsFile;
	lessonsFile.open("lessons.dat", ios::app | ios::binary | ios::in);
	while (!lessonsFile.eof())
	{
		char magic = ' ';
		lessonsFile.read(&magic, 1);
		if (magic != 'L') break;

		float priority;
		lessonsFile.read((char*)&priority, 4);

		vector<float> position;
		for (int j = 0; j < field_h; j++) {
			for (int i = 0; i < field_w; i++) {
				for (int c = 0; c < 2; c++) {
					float f;
					lessonsFile.read((char*)&f, 4);
					position.push_back(f);
				}
			}
		}

		usefulLessons.push_back(
			Lesson(
				Table(position, field_w, field_h), 
				field_w, field_h, 
				priority
			)
		);
	}
	lessonsFile.close();

/*	for (int i = 0; i < usefulLessons.size(); i++)
	{
		printField(usefulLessons[i].position.vals, usefulLessons[i].position.width, usefulLessons[i].position.height);
	}*/

	printf("Lessons in the book: %d\n", usefulLessons.size());

	// 1. Generating training & testing data

	vector<vec_t> train_input_data;
	vector<vec_t> train_output_data;

	// Adding the lessons
	for (int k = 0; k < usefulLessons.size(); ++k)
	{
		vec_t pos_item = usefulLessons[k].position.toVec();
		train_input_data.push_back(pos_item);

		vec_t pri_item { usefulLessons[k].priority };
		train_output_data.push_back(pri_item);
	}

	// Adding some fake lessons
	for (int k = 0; k < usefulLessons.size(); ++k)
	{
		Lesson falseLesson = usefulLessons[k];
		falseLesson.priority = 0.0f; // It is false
		falseLesson.position = falseLesson.position.translateRoll(
			rand() % (falseLesson.field_w - 2) + 1,
			rand() % (falseLesson.field_h - 2) + 1
		);

		train_input_data.push_back(falseLesson.position.toVec());

		vec_t pri_item { 0.0 };
		train_output_data.push_back(pri_item);
	}

	printf("Training...\n");

	size_t batch_size = std::min((int)train_input_data.size(), 20);//training_batch;
	double loss = 0; int ee = 0;
	
	double delta_loss_per_epoch;
	
	//gradient_descent opt; opt.alpha = 0.75;
	adam opt; opt.alpha /= 10;
	//int succeeded_tests;

	size_t epochs = 100;
	loss = net.get_loss<mse>(train_input_data, train_output_data);
	do
	{
		net.fit<mse>(opt, train_input_data, train_output_data, batch_size, epochs);

		double old_loss = loss;
		loss = net.get_loss<mse>(train_input_data, train_output_data);

		delta_loss_per_epoch = (old_loss - loss) / epochs;
		//if (delta_loss_per_epoch < 0) opt.alpha /= 2;

		// Scoring

		int succeeded_tests = 0;
		for (int i = 0; i < usefulLessons.size(); i++)
		{
			Table field_data = usefulLessons[i].position;

			Point move = makeMove(net, field_data, false);

			bool hit_the_point = 
				move.i == (field_data.width - 1) / 2 &&
				move.j == (field_data.height - 1) / 2;

			if ((usefulLessons[i].priority > 0 && hit_the_point) || 
				(usefulLessons[i].priority < 0 && !hit_the_point))
			{
				succeeded_tests++;
				printf("SUCCESS\n");
				printField(usefulLessons[i].position.vals,usefulLessons[i].position.width, usefulLessons[i].position.height);
			}
			else
			{
				printf("FAIL\n");
				printField(usefulLessons[i].position.vals, usefulLessons[i].position.width, usefulLessons[i].position.height);
			}
		}

		ee += epochs;
		cout << "epoch " << ee << ": loss=" << loss << " dloss=" << delta_loss_per_epoch << "; alpha=" << opt.alpha << "; learned: " << succeeded_tests << " of " << usefulLessons.size() << "(" << (int)(succeeded_tests * 100 / usefulLessons.size()) << "%)" << endl;

	} while (delta_loss_per_epoch > mse_stop/* || delta_loss_per_epoch < 0*/);
}

int main(int argc, char** argv)
{
	std::cout << "NN backend: " << core::default_engine() << std::endl;

	const int field_w = 7, field_h = 7, vic_line_len = 4;

	network<sequential> net;
	try
	{
		net.load("xoxonet.weights");
		printf("Net loaded\n");
	}
	catch (nn_error&)
	{
		printf("Can't load the net. Creating a new one\n");

		int size = field_w * field_h;

/*		int maps = 50;

		net <<
			layers::conv(field_w, field_h, 3, 1, maps) <<
			layers::fc(maps, 2 * maps) << tanh_layer(2 * maps) <<
			layers::fc(2 * maps, 2 * maps) << tanh_layer(2 * maps) <<
			layers::fc(2 * maps, maps) << tanh_layer(maps) <<
			layers::deconv(1, 1, 3, maps, 1);

			420 of 736
			*/

		int conv_kernel = 2;
		int conv1_out_w = field_w - conv_kernel + 1;
		int conv1_out_h = field_h - conv_kernel + 1;
		int conv1_out = conv1_out_w * conv1_out_h;
		int maps = 2 * conv_kernel * conv_kernel;	// from the ceiling

		int conv_kernel2 = 4;
		int conv2_out_w = conv1_out_w - conv_kernel2 + 1;
		int conv2_out_h = conv1_out_h - conv_kernel2 + 1;
		int conv2_out = conv2_out_w * conv2_out_h;
		int maps2 = maps * conv_kernel2 * conv_kernel2;	// from the ceiling




		net << 
/*			layers::fc(size * 2, size * 2) << tanh_layer(size * 2) <<
			layers::fc(size * 2, size * 2) << tanh_layer(size * 2) <<*/

			layers::conv(field_w, field_h, conv_kernel, 2, maps) <<

			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<
			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<
			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<
			layers::fc(conv1_out * maps, conv1_out * maps) << tanh_layer(conv1_out * maps) <<

			layers::conv(conv1_out_w, conv1_out_h, conv_kernel2, maps, maps2) <<

			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<
			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<
			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<
			layers::fc(conv2_out * maps2, conv2_out * maps2) << tanh_layer(conv2_out * maps2) <<

			layers::fc(conv2_out * maps2, 1) << tanh_layer(1);
	}

	if (argc == 2 && strcmp(argv[1], "train-first") == 0)
	{
		train(field_w, field_h, net, 1e-4);
	}


	vector<Lesson> lessons[2];	// Each player's lessons

	Table field_data(field_w, field_h);
	for (int j = 0; j < field_h; j++) {
		for (int i = 0; i < field_w; i++) {
			for (int c = 0; c < 2; c++) {
				field_data.vals.push_back(0.0);
			}
		}
	}

	srand(time(nullptr));
	int userPlayerIndex = rand() % 2;

	int victor = -2; int moveId = 0;
	do {
		for (int pI = 0; pI < 2; pI++) {
			printField(field_data.vals, field_w, field_h);
			int vic = checkVictory(field_data.vals, field_w, field_h, vic_line_len);
			if (vic == 0)
			{
				printf("Player X wins\n");
				victor = 0;
				break;
			}
			else if (vic == 1)
			{
				printf("Player O wins\n");
				victor = 1;
				break;
			}

			Table field_data_me_him = field_data;
			if (pI != 0) {
				// If the current player isn't "X", we inverse the field because "X" should mean "me"
				field_data_me_him = field_data_me_him.inverseChannels();
			}

			Point move(0, 0, field_w, field_h);
			if (pI == userPlayerIndex)
			{
				// Player move
				do {
					printf("%c>", playerSymbol(pI));
					char x = getchar();
					char y = getchar();
					char c;  while ((c = getchar()) != '\n' && c != EOF) {}
					move.i = x - 'a';
					move.j = y - '1';
				} while (move.i >= field_w || move.j > field_h || move.i < 0 || move.j < 0);

			}
			else
			{
				// AI move
				move = makeMove(net, field_data_me_him);
			}

			if (victor != -1) {
				// If not draw
				if (moveId > 0)
				{
					lessons[pI].push_back(Lesson(field_data_me_him, field_w, field_h, move, 1.0));
				}
				field_data.vals[(move.j * field_w + move.i) * 2 + pI] = 1.0;
			}
			moveId++;
		}

	} while (victor < -1);

	if (victor == userPlayerIndex) {
		printf("Other player wins. I shall learn\n");

		vector<Lesson> usefulLessons;
		float priority = 1.0;
		for (int k = lessons[victor].size() - 1; k >= 0; k--)
		{
			Lesson cur = lessons[victor][k].setPriority(priority);
			
			// In the book 1 means "me" and -1 means "other player", not "X" and "O"
			//if (victor != 0 /* X */) cur = cur.inverse();

			usefulLessons.push_back(cur);

			//Lesson curi = cur.inverse().mulPriority(0.95);		// Defense is less prioritized than attack
			//usefulLessons.push_back(curi);
			
			priority /= 1.2;
		}

		
		/*int looser = (victor + 1) % 2;
		priority = -1;
		int k = lessons[looser].size() - 1;
		{
			Lesson cur = lessons[looser][k].setPriority(priority);
			usefulLessons.push_back(cur);

			priority /= 1.2;
		}*/
		
		//usefulLessons.push_back(Lesson(Table::empty(field_w, field_h), field_w, field_h, 0.0));

		// Saving useful lessons to file
		ofstream lessonsFile;
		lessonsFile.open("lessons.dat", ios::app | ios::binary | ios::out);
		for (int k = 0; k < usefulLessons.size(); k++)
		{
			lessonsFile.write("L", 1);	// Magic for a lesson

			lessonsFile.write((const char*) &(usefulLessons[k].priority), 4);
			for (int j = 0; j < field_h; j++) {
				for (int i = 0; i < field_w; i++) {
					for (int c = 0; c < 2; c++) {
						lessonsFile.write((const char*) &(usefulLessons[k].position.vals[(j * field_w + i) * 2 + c]), 4);
					}
				}
			}
		}
		lessonsFile.close();

		printf("%d lessons appended to the book\n", usefulLessons.size());

		train(field_w, field_h, net, 1e-4);

		printf("Saving net...");
		net.save("xoxonet.weights");
	}
	else if (victor == -1)
	{
		printf("Nothing to learn from draw");
	}
	else
	{
		printf("I win. I am a clever bot. No more learning today\n");
	}

    return 0;
}
