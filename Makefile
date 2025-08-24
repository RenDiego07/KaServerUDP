
DEV = lo
HANDLE = root
NETEM = netem
PARAMS = delay 100ms 20ms loss 5% duplicate 20% seed 785877279124452776

.PHONY: netem-on netem-off netem-show

netem-on:
	@echo "⚙️  Activando netem en $(DEV) con parámetros: $(PARAMS)"
	sudo tc qdisc add dev $(DEV) $(HANDLE) $(NETEM) $(PARAMS) || true
	sudo tc qdisc change dev $(DEV) $(HANDLE) $(NETEM) $(PARAMS)

netem-off:
	@echo "🧹 Eliminando configuración netem en $(DEV)"
	sudo tc qdisc del dev $(DEV) $(HANDLE) || true

netem-show:
	@echo "🔎 Estado actual de netem en $(DEV):"
	tc qdisc show dev $(DEV)
