p4dtg-config:
	MapTab::runPushed :
		Check for config/svc-MAP
		Invoke p4dtg-service -install MAP

	MapTab::deletePushed :
		Check for repl/run-MAP
		Invoke p4dtg-service -remove MAP

p4dtg-service:
	-install MAP :
		Check for config/svc-MAP
		Check for config/map-MAP 
		Check for repl/run-MAP 
		Install service p4dtg-service-MAP
		Create config/svc-MAP

	-remove MAP :
		Stop service p4tdg-service-MAP
		Remove service p4dtg-service-MAP
		Remove config/svc-MAP

	status:
		Check for repl/run-MAP
		Check for repl/stop-MAP
		Report status

	start :
		Check for repl/run-MAP
		Invoke p4dtg-repl MAP PATH

	stop :
		Check for repl/run-MAP
		Create repl/stop-MAP
		Wait for repl/run-MAP to vanish
