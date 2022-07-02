class Singleton
{
private:
    Singleton() {};
    ~Singleton() {};
    Singleton(const Singleton&);
    Singleton& operator=(const Singleton&);
public:
    static Singleton& GetInstance()
    {
        static Singleton instance;
        return instance;
    }

};

// eager singleton
// class Singleton
// {
// private:
// 	static Singleton instance;
// private:
// 	Singleton();
// 	~Singleton();
// 	Singleton(const Singleton&);
// 	Singleton& operator=(const Singleton&);
// public:
// 	static Singleton& getInstance() {
// 		return instance;
// 	}
// }

// // initialize defaultly
// Singleton Singleton::instance;